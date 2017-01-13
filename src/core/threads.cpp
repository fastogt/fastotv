#include "core/threads.h"

#include <common/utils.h>

#include "core/video_state.h"
#include "core/utils.h"
#include "core/types.h"

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25

namespace {
void print_error(const char* filename, int err) {
  char errbuf[128];
  const char* errbuf_ptr = errbuf;

  if (av_strerror(err, errbuf, sizeof(errbuf)) < 0) {
    errbuf_ptr = strerror(AVUNERROR(err));
  }
  av_log(NULL, AV_LOG_ERROR, "%s: %s\n", filename, errbuf_ptr);
}

int decode_interrupt_cb(void* user_data) {
  VideoState* is = static_cast<VideoState*>(user_data);
  return is->abort_request;
}

int is_realtime(AVFormatContext* s) {
  if (!strcmp(s->iformat->name, "rtp") || !strcmp(s->iformat->name, "rtsp") ||
      !strcmp(s->iformat->name, "sdp")) {
    return 1;
  }

  if (s->pb && (!strncmp(s->filename, "rtp:", 4) || !strncmp(s->filename, "udp:", 4))) {
    return 1;
  }
  return 0;
}

int stream_has_enough_packets(AVStream* st, int stream_id, PacketQueue* queue) {
  return stream_id < 0 || queue->abortRequest() ||
         (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
         (queue->nbPackets() > MIN_FRAMES &&
          (!queue->duration() || av_q2d(st->time_base) * queue->duration() > 1.0));
}

int queue_picture(VideoState* is,
                  AVFrame* src_frame,
                  double pts,
                  double duration,
                  int64_t pos,
                  int serial) {
#if defined(DEBUG_SYNC)
  printf("frame_type=%c pts=%0.3f\n", av_get_picture_type_char(src_frame->pict_type), pts);
#endif

  Frame* vp = is->pictq->peek_writable();
  if (!vp) {
    return ERROR_RESULT_VALUE;
  }

  vp->sar = src_frame->sample_aspect_ratio;
  vp->uploaded = 0;

  /* alloc or resize hardware picture buffer */
  if (!vp->bmp || !vp->allocated || vp->width != src_frame->width ||
      vp->height != src_frame->height || vp->format != src_frame->format) {
    SDL_Event event;

    vp->allocated = 0;
    vp->width = src_frame->width;
    vp->height = src_frame->height;
    vp->format = src_frame->format;

    /* the allocation must be done in the main thread to avoid
       locking problems. */
    event.type = FF_ALLOC_EVENT;
    event.user.data1 = is;
    SDL_PushEvent(&event);

    /* wait until the picture is allocated */
    SDL_LockMutex(is->pictq->mutex);
    while (!vp->allocated && !is->videoq->abortRequest()) {
      SDL_CondWait(is->pictq->cond, is->pictq->mutex);
    }
    /* if the queue is aborted, we have to pop the pending ALLOC event or wait for the allocation to
     * complete */
    if (is->videoq->abortRequest() &&
        SDL_PeepEvents(&event, 1, SDL_GETEVENT, FF_ALLOC_EVENT, FF_ALLOC_EVENT) != 1) {
      while (!vp->allocated && !is->abort_request) {
        SDL_CondWait(is->pictq->cond, is->pictq->mutex);
      }
    }
    SDL_UnlockMutex(is->pictq->mutex);

    if (is->videoq->abortRequest())
      return -1;
  }

  /* if the frame is not skipped, then display it */
  if (vp->bmp) {
    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;
    vp->serial = serial;

    av_frame_move_ref(vp->frame, src_frame);
    is->pictq->push();
  }
  return 0;
}

int cmp_audio_fmts(enum AVSampleFormat fmt1,
                   int64_t channel_count1,
                   enum AVSampleFormat fmt2,
                   int64_t channel_count2) {
  /* If channel count == 1, planar and non-planar formats are the same */
  if (channel_count1 == 1 && channel_count2 == 1)
    return av_get_packed_sample_fmt(fmt1) != av_get_packed_sample_fmt(fmt2);
  else
    return channel_count1 != channel_count2 || fmt1 != fmt2;
}

int get_video_frame(VideoState* is, AVFrame* frame) {
  int got_picture;

  if ((got_picture = is->viddec->decodeFrame(frame, NULL)) < 0) {
    return -1;
  }

  if (got_picture) {
    double dpts = NAN;

    if (frame->pts != AV_NOPTS_VALUE)
      dpts = av_q2d(is->video_st->time_base) * frame->pts;

    frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);

    if (is->opt->framedrop > 0 ||
        (is->opt->framedrop && is->get_master_sync_type() != AV_SYNC_VIDEO_MASTER)) {
      if (frame->pts != AV_NOPTS_VALUE) {
        double diff = dpts - is->get_master_clock();
        if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
            diff - is->frame_last_filter_delay < 0 &&
            is->viddec->pktSerial() == is->vidclk->serial() && is->videoq->nbPackets()) {
          is->frame_drops_early++;
          av_frame_unref(frame);
          got_picture = 0;
        }
      }
    }
  }

  return got_picture;
}
}

/* this thread gets the stream from the disk or the network */
int read_thread(void* user_data) {
  VideoState* is = static_cast<VideoState*>(user_data);
  AVFormatContext* ic = NULL;
  int err, i, ret;
  int st_index[AVMEDIA_TYPE_NB];
  AVPacket pkt1, *pkt = &pkt1;
  int64_t stream_start_time;
  int pkt_in_play_range = 0;
  AVDictionaryEntry* t;
  AVDictionary** opts;
  int orig_nb_streams;
  SDL_mutex* wait_mutex = SDL_CreateMutex();
  int scan_all_pmts_set = 0;
  int64_t pkt_ts;

  const char* in_filename = common::utils::c_strornull(is->opt->input_filename);
  if (!wait_mutex) {
    av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
    ret = AVERROR(ENOMEM);
    goto fail;
  }

  memset(st_index, -1, sizeof(st_index));
  is->last_video_stream = is->video_stream = -1;
  is->last_audio_stream = is->audio_stream = -1;
  is->last_subtitle_stream = is->subtitle_stream = -1;
  is->eof = 0;

  ic = avformat_alloc_context();
  if (!ic) {
    av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
    ret = AVERROR(ENOMEM);
    goto fail;
  }
  ic->interrupt_callback.callback = decode_interrupt_cb;
  ic->interrupt_callback.opaque = is;
  if (!av_dict_get(is->copt->format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
    av_dict_set(&is->copt->format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
    scan_all_pmts_set = 1;
  }
  err = avformat_open_input(&ic, in_filename, is->iformat, &is->copt->format_opts);
  if (err < 0) {
    print_error(in_filename, err);
    ret = -1;
    goto fail;
  }
  if (scan_all_pmts_set)
    av_dict_set(&is->copt->format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);

  if ((t = av_dict_get(is->copt->format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
    av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
    ret = AVERROR_OPTION_NOT_FOUND;
    goto fail;
  }
  is->ic = ic;

  if (is->opt->genpts) {
    ic->flags |= AVFMT_FLAG_GENPTS;
  }

  av_format_inject_global_side_data(ic);

  opts = setup_find_stream_info_opts(ic, is->copt->codec_opts);
  orig_nb_streams = ic->nb_streams;

  err = avformat_find_stream_info(ic, opts);

  for (i = 0; i < orig_nb_streams; i++) {
    av_dict_free(&opts[i]);
  }
  av_freep(&opts);

  if (err < 0) {
    av_log(NULL, AV_LOG_WARNING, "%s: could not find codec parameters\n", in_filename);
    ret = -1;
    goto fail;
  }

  if (ic->pb)
    ic->pb->eof_reached =
        0;  // FIXME hack, ffplay maybe should not use avio_feof() to test for the end

  if (is->opt->seek_by_bytes < 0) {
    is->opt->seek_by_bytes =
        !!(ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", ic->iformat->name);
  }

  is->max_frame_duration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

  if (is->opt->window_title.empty() && (t = av_dict_get(ic->metadata, "title", NULL, 0))) {
    is->opt->window_title = av_asprintf("%s - %s", t->value, in_filename);
  }

  /* if seeking requested, we execute it */
  if (is->opt->start_time != AV_NOPTS_VALUE) {
    int64_t timestamp;

    timestamp = is->opt->start_time;
    /* add the stream start time */
    if (ic->start_time != AV_NOPTS_VALUE)
      timestamp += ic->start_time;
    ret = avformat_seek_file(ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
    if (ret < 0) {
      av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n", in_filename,
             (double)timestamp / AV_TIME_BASE);
    }
  }

  is->realtime = is_realtime(ic);

  if (is->opt->show_status) {
    av_dump_format(ic, 0, in_filename, 0);
  }

  for (i = 0; i < ic->nb_streams; i++) {
    AVStream* st = ic->streams[i];
    enum AVMediaType type = st->codecpar->codec_type;
    st->discard = AVDISCARD_ALL;
    const char* want_spec = common::utils::c_strornull(is->opt->wanted_stream_spec[type]);
    if (type >= 0 && want_spec && st_index[type] == -1) {
      if (avformat_match_stream_specifier(ic, st, want_spec) > 0) {
        st_index[type] = i;
      }
    }
  }
  for (i = 0; i < static_cast<int>(AVMEDIA_TYPE_NB); i++) {
    const char* want_spec = common::utils::c_strornull(is->opt->wanted_stream_spec[i]);
    if (want_spec && st_index[i] == -1) {
      av_log(NULL, AV_LOG_ERROR, "Stream specifier %s does not match any %s stream\n", want_spec,
             av_get_media_type_string(static_cast<AVMediaType>(i)));
      st_index[i] = INT_MAX;
    }
  }

  if (!is->opt->video_disable) {
    st_index[AVMEDIA_TYPE_VIDEO] =
        av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
  }
  if (!is->opt->audio_disable) {
    st_index[AVMEDIA_TYPE_AUDIO] =
        av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, st_index[AVMEDIA_TYPE_AUDIO],
                            st_index[AVMEDIA_TYPE_VIDEO], NULL, 0);
  }
  if (!is->opt->video_disable && !is->opt->subtitle_disable) {
    st_index[AVMEDIA_TYPE_SUBTITLE] =
        av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE, st_index[AVMEDIA_TYPE_SUBTITLE],
                            (st_index[AVMEDIA_TYPE_AUDIO] >= 0 ? st_index[AVMEDIA_TYPE_AUDIO]
                                                               : st_index[AVMEDIA_TYPE_VIDEO]),
                            NULL, 0);
  }

  if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
    AVStream* st = ic->streams[st_index[AVMEDIA_TYPE_VIDEO]];
    AVCodecParameters* codecpar = st->codecpar;
    AVRational sar = av_guess_sample_aspect_ratio(ic, st, NULL);
    if (codecpar->width) {
      is->set_default_window_size(codecpar->width, codecpar->height, sar);
    }
  }

  /* open the streams */
  if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
    is->stream_component_open(st_index[AVMEDIA_TYPE_AUDIO]);
  }

  ret = -1;
  if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
    ret = is->stream_component_open(st_index[AVMEDIA_TYPE_VIDEO]);
  }
  if (is->opt->show_mode == SHOW_MODE_NONE) {
    is->opt->show_mode = ret >= 0 ? SHOW_MODE_VIDEO : SHOW_MODE_RDFT;
  }

  if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
    is->stream_component_open(st_index[AVMEDIA_TYPE_SUBTITLE]);
  }

  if (is->video_stream < 0 && is->audio_stream < 0) {
    av_log(NULL, AV_LOG_FATAL, "Failed to open file '%s' or configure filtergraph\n", in_filename);
    ret = -1;
    goto fail;
  }

  if (is->opt->infinite_buffer < 0 && is->realtime) {
    is->opt->infinite_buffer = 1;
  }

  while (true) {
    if (is->abort_request)
      break;
    if (is->paused != is->last_paused) {
      is->last_paused = is->paused;
      if (is->paused)
        is->read_pause_return = av_read_pause(ic);
      else
        av_read_play(ic);
    }
#if CONFIG_RTSP_DEMUXER || CONFIG_MMSH_PROTOCOL
    if (is->paused &&
        (!strcmp(ic->iformat->name, "rtsp") || (ic->pb && !strncmp(input_filename, "mmsh:", 5)))) {
      /* wait 10 ms to avoid trying to get another packet */
      /* XXX: horrible */
      SDL_Delay(10);
      continue;
    }
#endif
    if (is->seek_req) {
      int64_t seek_target = is->seek_pos;
      int64_t seek_min = is->seek_rel > 0 ? seek_target - is->seek_rel + 2 : INT64_MIN;
      int64_t seek_max = is->seek_rel < 0 ? seek_target - is->seek_rel - 2 : INT64_MAX;
      // FIXME the +-2 is due to rounding being not done in the correct direction in generation
      //      of the seek_pos/seek_rel variables

      ret = avformat_seek_file(is->ic, -1, seek_min, seek_target, seek_max, is->seek_flags);
      if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s: error while seeking\n", is->ic->filename);
      } else {
        if (is->audio_stream >= 0) {
          is->audioq->flush();
          is->audioq->put(PacketQueue::flush_pkt());
        }
        if (is->subtitle_stream >= 0) {
          is->subtitleq->flush();
          is->subtitleq->put(PacketQueue::flush_pkt());
        }
        if (is->video_stream >= 0) {
          is->videoq->flush();
          is->videoq->put(PacketQueue::flush_pkt());
        }
        if (is->seek_flags & AVSEEK_FLAG_BYTE) {
          is->extclk->set_clock(NAN, 0);
        } else {
          is->extclk->set_clock(seek_target / (double)AV_TIME_BASE, 0);
        }
      }
      is->seek_req = 0;
      is->queue_attachments_req = 1;
      is->eof = 0;
      if (is->paused) {
        is->step_to_next_frame();
      }
    }
    if (is->queue_attachments_req) {
      if (is->video_st && is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
        AVPacket copy;
        if ((ret = av_copy_packet(&copy, &is->video_st->attached_pic)) < 0) {
          goto fail;
        }
        is->videoq->put(&copy);
        is->videoq->put_nullpacket(is->video_stream);
      }
      is->queue_attachments_req = 0;
    }

    /* if the queue are full, no need to read more */
    if (is->opt->infinite_buffer < 1 &&
        (is->audioq->size() + is->videoq->size() + is->subtitleq->size() > MAX_QUEUE_SIZE ||
         (stream_has_enough_packets(is->audio_st, is->audio_stream, is->audioq) &&
          stream_has_enough_packets(is->video_st, is->video_stream, is->videoq) &&
          stream_has_enough_packets(is->subtitle_st, is->subtitle_stream, is->subtitleq)))) {
      /* wait 10 ms */
      SDL_LockMutex(wait_mutex);
      SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
      SDL_UnlockMutex(wait_mutex);
      continue;
    }
    if (!is->paused && (!is->audio_st || (is->auddec->finished == is->audioq->serial &&
                                          is->sampq->nb_remaining() == 0)) &&
        (!is->video_st ||
         (is->viddec->finished == is->videoq->serial && is->pictq->nb_remaining() == 0))) {
      if (is->opt->loop != 1 && (!is->opt->loop || --is->opt->loop)) {
        is->stream_seek(is->opt->start_time != AV_NOPTS_VALUE ? is->opt->start_time : 0, 0, 0);
      } else if (is->opt->autoexit) {
        ret = AVERROR_EOF;
        goto fail;
      }
    }
    ret = av_read_frame(ic, pkt);
    if (ret < 0) {
      if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof) {
        if (is->video_stream >= 0) {
          is->videoq->put_nullpacket(is->video_stream);
        }
        if (is->audio_stream >= 0) {
          is->audioq->put_nullpacket(is->audio_stream);
        }
        if (is->subtitle_stream >= 0) {
          is->subtitleq->put_nullpacket(is->subtitle_stream);
        }
        is->eof = 1;
      }
      if (ic->pb && ic->pb->error)
        break;
      SDL_LockMutex(wait_mutex);
      SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
      SDL_UnlockMutex(wait_mutex);
      continue;
    } else {
      is->eof = 0;
    }
    /* check if packet is in play range specified by user, then queue, otherwise discard */
    stream_start_time = ic->streams[pkt->stream_index]->start_time;
    pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
    pkt_in_play_range =
        is->opt->duration == AV_NOPTS_VALUE ||
        (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
                    av_q2d(ic->streams[pkt->stream_index]->time_base) -
                (double)(is->opt->start_time != AV_NOPTS_VALUE ? is->opt->start_time : 0) /
                    1000000 <=
            ((double)is->opt->duration / 1000000);
    if (pkt->stream_index == is->audio_stream && pkt_in_play_range) {
      is->audioq->put(pkt);
    } else if (pkt->stream_index == is->video_stream && pkt_in_play_range &&
               !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
      is->videoq->put(pkt);
    } else if (pkt->stream_index == is->subtitle_stream && pkt_in_play_range) {
      is->subtitleq->put(pkt);
    } else {
      av_packet_unref(pkt);
    }
  }

  ret = 0;
fail:
  if (ic && !is->ic)
    avformat_close_input(&ic);

  if (ret != 0) {
    SDL_Event event;

    event.type = FF_QUIT_EVENT;
    event.user.data1 = is;
    SDL_PushEvent(&event);
  }
  SDL_DestroyMutex(wait_mutex);
  return 0;
}

int audio_thread(void* user_data) {
  VideoState* is = static_cast<VideoState*>(user_data);
  Frame* af;
#if CONFIG_AVFILTER
  int last_serial = -1;
  int64_t dec_channel_layout;
  int reconfigure;
#endif
  int got_frame = 0;
  int ret = 0;

  AVFrame* frame = av_frame_alloc();
  if (!frame) {
    return AVERROR(ENOMEM);
  }

  do {
    if ((got_frame = is->auddec->decodeFrame(frame, NULL)) < 0) {
      goto the_end;
    }

    if (got_frame) {
      AVRational tb = {1, frame->sample_rate};

#if CONFIG_AVFILTER
      dec_channel_layout =
          get_valid_channel_layout(frame->channel_layout, av_frame_get_channels(frame));

      reconfigure = cmp_audio_fmts(is->audio_filter_src.fmt, is->audio_filter_src.channels,
                                   (AVSampleFormat)frame->format, av_frame_get_channels(frame)) ||
                    is->audio_filter_src.channel_layout != dec_channel_layout ||
                    is->audio_filter_src.freq != frame->sample_rate ||
                    is->auddec->pktSerial() != last_serial;

      if (reconfigure) {
        char buf1[1024], buf2[1024];
        av_get_channel_layout_string(buf1, sizeof(buf1), -1, is->audio_filter_src.channel_layout);
        av_get_channel_layout_string(buf2, sizeof(buf2), -1, dec_channel_layout);
        av_log(NULL, AV_LOG_DEBUG,
               "Audio frame changed from rate:%d ch:%d fmt:%s layout:%s serial:%d to rate:%d ch:%d "
               "fmt:%s layout:%s serial:%d\n",
               is->audio_filter_src.freq, is->audio_filter_src.channels,
               av_get_sample_fmt_name(is->audio_filter_src.fmt), buf1, last_serial,
               frame->sample_rate, av_frame_get_channels(frame),
               av_get_sample_fmt_name((AVSampleFormat)frame->format), buf2,
               is->auddec->pktSerial());

        is->audio_filter_src.fmt = static_cast<AVSampleFormat>(frame->format);
        is->audio_filter_src.channels = av_frame_get_channels(frame);
        is->audio_filter_src.channel_layout = dec_channel_layout;
        is->audio_filter_src.freq = frame->sample_rate;
        last_serial = is->auddec->pktSerial();

        if ((ret = is->configure_audio_filters(is->opt->afilters, 1)) < 0) {
          goto the_end;
        }
      }

      if ((ret = av_buffersrc_add_frame(is->in_audio_filter, frame)) < 0) {
        goto the_end;
      }

      while ((ret = av_buffersink_get_frame_flags(is->out_audio_filter, frame, 0)) >= 0) {
        tb = is->out_audio_filter->inputs[0]->time_base;
#endif
        if (!(af = is->sampq->peek_writable())) {
          goto the_end;
        }

        af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
        af->pos = av_frame_get_pkt_pos(frame);
        af->serial = is->auddec->pktSerial();
        AVRational tmp = {frame->nb_samples, frame->sample_rate};
        af->duration = av_q2d(tmp);

        av_frame_move_ref(af->frame, frame);
        is->sampq->push();

#if CONFIG_AVFILTER
        if (is->audioq->serial != is->auddec->pktSerial()) {
          break;
        }
      }
      if (ret == AVERROR_EOF) {
        is->auddec->finished = is->auddec->pktSerial();
      }
#endif
    }
  } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
the_end:
#if CONFIG_AVFILTER
  avfilter_graph_free(&is->agraph);
#endif
  av_frame_free(&frame);
  return ret;
}

int video_thread(void* user_data) {
  VideoState* is = static_cast<VideoState*>(user_data);
  AVFrame* frame = av_frame_alloc();
  double pts;
  double duration;
  int ret;
  AVRational tb = is->video_st->time_base;
  AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);

#if CONFIG_AVFILTER
  AVFilterGraph* graph = avfilter_graph_alloc();
  AVFilterContext *filt_out = NULL, *filt_in = NULL;
  int last_w = 0;
  int last_h = 0;
  enum AVPixelFormat last_format = AV_PIX_FMT_NONE;  //-2
  int last_serial = -1;
  int last_vfilter_idx = 0;
  if (!graph) {
    av_frame_free(&frame);
    return AVERROR(ENOMEM);
  }

#endif

  if (!frame) {
#if CONFIG_AVFILTER
    avfilter_graph_free(&graph);
#endif
    return AVERROR(ENOMEM);
  }

  while (true) {
    ret = get_video_frame(is, frame);
    if (ret < 0) {
      goto the_end;
    }
    if (!ret) {
      continue;
    }

#if CONFIG_AVFILTER
    if (last_w != frame->width || last_h != frame->height || last_format != frame->format ||
        last_serial != is->viddec->pktSerial() || last_vfilter_idx != is->vfilter_idx) {
      av_log(NULL, AV_LOG_DEBUG,
             "Video frame changed from size:%dx%d format:%s serial:%d to size:%dx%d format:%s "
             "serial:%d\n",
             last_w, last_h, (const char*)av_x_if_null(av_get_pix_fmt_name(last_format), "none"),
             last_serial, frame->width, frame->height,
             (const char*)av_x_if_null(av_get_pix_fmt_name((AVPixelFormat)frame->format), "none"),
             is->viddec->pktSerial());
      avfilter_graph_free(&graph);
      graph = avfilter_graph_alloc();
      const char* vfilters = NULL;
      if (!is->opt->vfilters_list.empty()) {
        vfilters = is->opt->vfilters_list[is->vfilter_idx].c_str();
      }
      if ((ret = is->configure_video_filters(graph, vfilters, frame)) < 0) {
        SDL_Event event;
        event.type = FF_QUIT_EVENT;
        event.user.data1 = is;
        SDL_PushEvent(&event);
        goto the_end;
      }
      filt_in = is->in_video_filter;
      filt_out = is->out_video_filter;
      last_w = frame->width;
      last_h = frame->height;
      last_format = static_cast<AVPixelFormat>(frame->format);
      last_serial = is->viddec->pktSerial();
      last_vfilter_idx = is->vfilter_idx;
      frame_rate = filt_out->inputs[0]->frame_rate;
    }

    ret = av_buffersrc_add_frame(filt_in, frame);
    if (ret < 0)
      goto the_end;

    while (ret >= 0) {
      is->frame_last_returned_time = av_gettime_relative() / 1000000.0;

      ret = av_buffersink_get_frame_flags(filt_out, frame, 0);
      if (ret < 0) {
        if (ret == AVERROR_EOF) {
          is->viddec->finished = is->viddec->pktSerial();
        }
        ret = 0;
        break;
      }

      is->frame_last_filter_delay =
          av_gettime_relative() / 1000000.0 - is->frame_last_returned_time;
      if (fabs(is->frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0)
        is->frame_last_filter_delay = 0;
      tb = filt_out->inputs[0]->time_base;
#endif
      duration =
          (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num})
                                            : 0);
      pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
      ret = queue_picture(is, frame, pts, duration, av_frame_get_pkt_pos(frame),
                          is->viddec->pktSerial());
      av_frame_unref(frame);
#if CONFIG_AVFILTER
    }
#endif

    if (ret < 0)
      goto the_end;
  }
the_end:
#if CONFIG_AVFILTER
  avfilter_graph_free(&graph);
#endif
  av_frame_free(&frame);
  return 0;
}

int subtitle_thread(void* user_data) {
  VideoState* is = static_cast<VideoState*>(user_data);
  FrameQueue* subpq = is->subpq;
  SubDecoder* subdec = is->subdec;
  while (true) {
    Frame* sp = subpq->peek_writable();
    if (!sp) {
      return 0;
    }

    int got_subtitle = subdec->decodeFrame(NULL, &sp->sub);
    if (got_subtitle < 0) {
      break;
    }

    double pts = 0;
    if (got_subtitle && sp->sub.format == 0) {
      if (sp->sub.pts != AV_NOPTS_VALUE) {
        pts = static_cast<double>(sp->sub.pts) / static_cast<double>(AV_TIME_BASE);
      }
      sp->pts = pts;
      sp->serial = subdec->pktSerial();
      sp->width = subdec->width();
      sp->height = subdec->height();
      sp->uploaded = 0;

      /* now we can update the picture count */
      subpq->push();
    } else if (got_subtitle) {
      avsubtitle_free(&sp->sub);
    }
  }
  return 0;
}
