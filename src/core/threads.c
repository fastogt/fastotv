#include "ffmpeg_config.h"

#include "core/threads.h"

#include "core/types.h"

static void frame_queue_push(FrameQueue* f) {
  if (++f->windex == f->max_size)
    f->windex = 0;
  SDL_LockMutex(f->mutex);
  f->size++;
  SDL_CondSignal(f->cond);
  SDL_UnlockMutex(f->mutex);
}

static Frame* frame_queue_peek_writable(FrameQueue* f) {
  /* wait until we have space to put a new frame */
  SDL_LockMutex(f->mutex);
  while (f->size >= f->max_size && !f->pktq->abort_request) {
    SDL_CondWait(f->cond, f->mutex);
  }
  SDL_UnlockMutex(f->mutex);

  if (f->pktq->abort_request)
    return NULL;

  return &f->queue[f->windex];
}

static int queue_picture(VideoState* is,
                         AVFrame* src_frame,
                         double pts,
                         double duration,
                         int64_t pos,
                         int serial) {
  Frame* vp;

#if defined(DEBUG_SYNC)
  printf("frame_type=%c pts=%0.3f\n", av_get_picture_type_char(src_frame->pict_type), pts);
#endif

  if (!(vp = frame_queue_peek_writable(&is->pictq)))
    return -1;

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
    SDL_LockMutex(is->pictq.mutex);
    while (!vp->allocated && !is->videoq.abort_request) {
      SDL_CondWait(is->pictq.cond, is->pictq.mutex);
    }
    /* if the queue is aborted, we have to pop the pending ALLOC event or wait for the allocation to
     * complete */
    if (is->videoq.abort_request &&
        SDL_PeepEvents(&event, 1, SDL_GETEVENT, FF_ALLOC_EVENT, FF_ALLOC_EVENT) != 1) {
      while (!vp->allocated && !is->abort_request) {
        SDL_CondWait(is->pictq.cond, is->pictq.mutex);
      }
    }
    SDL_UnlockMutex(is->pictq.mutex);

    if (is->videoq.abort_request)
      return -1;
  }

  /* if the frame is not skipped, then display it */
  if (vp->bmp) {
    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;
    vp->serial = serial;

    av_frame_move_ref(vp->frame, src_frame);
    frame_queue_push(&is->pictq);
  }
  return 0;
}

static inline int cmp_audio_fmts(enum AVSampleFormat fmt1,
                                 int64_t channel_count1,
                                 enum AVSampleFormat fmt2,
                                 int64_t channel_count2) {
  /* If channel count == 1, planar and non-planar formats are the same */
  if (channel_count1 == 1 && channel_count2 == 1)
    return av_get_packed_sample_fmt(fmt1) != av_get_packed_sample_fmt(fmt2);
  else
    return channel_count1 != channel_count2 || fmt1 != fmt2;
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
static int packet_queue_get(PacketQueue* q, AVPacket* pkt, int block, int* serial) {
  MyAVPacketList* pkt1;
  int ret;

  SDL_LockMutex(q->mutex);

  for (;;) {
    if (q->abort_request) {
      ret = -1;
      break;
    }

    pkt1 = q->first_pkt;
    if (pkt1) {
      q->first_pkt = pkt1->next;
      if (!q->first_pkt)
        q->last_pkt = NULL;
      q->nb_packets--;
      q->size -= pkt1->pkt.size + sizeof(*pkt1);
      q->duration -= pkt1->pkt.duration;
      *pkt = pkt1->pkt;
      if (serial)
        *serial = pkt1->serial;
      av_free(pkt1);
      ret = 1;
      break;
    } else if (!block) {
      ret = 0;
      break;
    } else {
      SDL_CondWait(q->cond, q->mutex);
    }
  }
  SDL_UnlockMutex(q->mutex);
  return ret;
}

static int decoder_decode_frame(Decoder* d, AVFrame* frame, AVSubtitle* sub) {
  int got_frame = 0;

  do {
    int ret = -1;

    if (d->queue->abort_request)
      return -1;

    if (!d->packet_pending || d->queue->serial != d->pkt_serial) {
      AVPacket pkt;
      do {
        if (d->queue->nb_packets == 0)
          SDL_CondSignal(d->empty_queue_cond);
        if (packet_queue_get(d->queue, &pkt, 1, &d->pkt_serial) < 0)
          return -1;
        if (pkt.data == flush_pkt.data) {
          avcodec_flush_buffers(d->avctx);
          d->finished = 0;
          d->next_pts = d->start_pts;
          d->next_pts_tb = d->start_pts_tb;
        }
      } while (pkt.data == flush_pkt.data || d->queue->serial != d->pkt_serial);
      av_packet_unref(&d->pkt);
      d->pkt_temp = d->pkt = pkt;
      d->packet_pending = 1;
    }

    switch (d->avctx->codec_type) {
      case AVMEDIA_TYPE_VIDEO:
        ret = avcodec_decode_video2(d->avctx, frame, &got_frame, &d->pkt_temp);
        if (got_frame) {
          if (decoder_reorder_pts == -1) {
            frame->pts = av_frame_get_best_effort_timestamp(frame);
          } else if (!decoder_reorder_pts) {
            frame->pts = frame->pkt_dts;
          }
        }
        break;
      case AVMEDIA_TYPE_AUDIO:
        ret = avcodec_decode_audio4(d->avctx, frame, &got_frame, &d->pkt_temp);
        if (got_frame) {
          AVRational tb = (AVRational){1, frame->sample_rate};
          if (frame->pts != AV_NOPTS_VALUE)
            frame->pts = av_rescale_q(frame->pts, av_codec_get_pkt_timebase(d->avctx), tb);
          else if (d->next_pts != AV_NOPTS_VALUE)
            frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
          if (frame->pts != AV_NOPTS_VALUE) {
            d->next_pts = frame->pts + frame->nb_samples;
            d->next_pts_tb = tb;
          }
        }
        break;
      case AVMEDIA_TYPE_SUBTITLE:
        ret = avcodec_decode_subtitle2(d->avctx, sub, &got_frame, &d->pkt_temp);
        break;
    }

    if (ret < 0) {
      d->packet_pending = 0;
    } else {
      d->pkt_temp.dts = d->pkt_temp.pts = AV_NOPTS_VALUE;
      if (d->pkt_temp.data) {
        if (d->avctx->codec_type != AVMEDIA_TYPE_AUDIO)
          ret = d->pkt_temp.size;
        d->pkt_temp.data += ret;
        d->pkt_temp.size -= ret;
        if (d->pkt_temp.size <= 0)
          d->packet_pending = 0;
      } else {
        if (!got_frame) {
          d->packet_pending = 0;
          d->finished = d->pkt_serial;
        }
      }
    }
  } while (!got_frame && !d->finished);

  return got_frame;
}

static int get_video_frame(VideoState* is, AVFrame* frame) {
  int got_picture;

  if ((got_picture = decoder_decode_frame(&is->viddec, frame, NULL)) < 0)
    return -1;

  if (got_picture) {
    double dpts = NAN;

    if (frame->pts != AV_NOPTS_VALUE)
      dpts = av_q2d(is->video_st->time_base) * frame->pts;

    frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);

    if (framedrop > 0 || (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) {
      if (frame->pts != AV_NOPTS_VALUE) {
        double diff = dpts - get_master_clock(is);
        if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
            diff - is->frame_last_filter_delay < 0 && is->viddec.pkt_serial == is->vidclk.serial &&
            is->videoq.nb_packets) {
          is->frame_drops_early++;
          av_frame_unref(frame);
          got_picture = 0;
        }
      }
    }
  }

  return got_picture;
}

int audio_thread(void* arg) {
  VideoState* is = arg;
  AVFrame* frame = av_frame_alloc();
  Frame* af;
#if CONFIG_AVFILTER
  int last_serial = -1;
  int64_t dec_channel_layout;
  int reconfigure;
#endif
  int got_frame = 0;
  AVRational tb;
  int ret = 0;

  if (!frame)
    return AVERROR(ENOMEM);

  do {
    if ((got_frame = decoder_decode_frame(&is->auddec, frame, NULL)) < 0)
      goto the_end;

    if (got_frame) {
      tb = (AVRational){1, frame->sample_rate};

#if CONFIG_AVFILTER
      dec_channel_layout =
          get_valid_channel_layout(frame->channel_layout, av_frame_get_channels(frame));

      reconfigure = cmp_audio_fmts(is->audio_filter_src.fmt, is->audio_filter_src.channels,
                                   frame->format, av_frame_get_channels(frame)) ||
                    is->audio_filter_src.channel_layout != dec_channel_layout ||
                    is->audio_filter_src.freq != frame->sample_rate ||
                    is->auddec.pkt_serial != last_serial;

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
               av_get_sample_fmt_name(frame->format), buf2, is->auddec.pkt_serial);

        is->audio_filter_src.fmt = frame->format;
        is->audio_filter_src.channels = av_frame_get_channels(frame);
        is->audio_filter_src.channel_layout = dec_channel_layout;
        is->audio_filter_src.freq = frame->sample_rate;
        last_serial = is->auddec.pkt_serial;

        if ((ret = configure_audio_filters(is, afilters, 1)) < 0)
          goto the_end;
      }

      if ((ret = av_buffersrc_add_frame(is->in_audio_filter, frame)) < 0)
        goto the_end;

      while ((ret = av_buffersink_get_frame_flags(is->out_audio_filter, frame, 0)) >= 0) {
        tb = is->out_audio_filter->inputs[0]->time_base;
#endif
        if (!(af = frame_queue_peek_writable(&is->sampq)))
          goto the_end;

        af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
        af->pos = av_frame_get_pkt_pos(frame);
        af->serial = is->auddec.pkt_serial;
        af->duration = av_q2d((AVRational){frame->nb_samples, frame->sample_rate});

        av_frame_move_ref(af->frame, frame);
        frame_queue_push(&is->sampq);

#if CONFIG_AVFILTER
        if (is->audioq.serial != is->auddec.pkt_serial)
          break;
      }
      if (ret == AVERROR_EOF)
        is->auddec.finished = is->auddec.pkt_serial;
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

int video_thread(void* arg) {
  VideoState* is = arg;
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
  enum AVPixelFormat last_format = -2;
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

  for (;;) {
    ret = get_video_frame(is, frame);
    if (ret < 0)
      goto the_end;
    if (!ret)
      continue;

#if CONFIG_AVFILTER
    if (last_w != frame->width || last_h != frame->height || last_format != frame->format ||
        last_serial != is->viddec.pkt_serial || last_vfilter_idx != is->vfilter_idx) {
      av_log(NULL, AV_LOG_DEBUG,
             "Video frame changed from size:%dx%d format:%s serial:%d to size:%dx%d format:%s "
             "serial:%d\n",
             last_w, last_h, (const char*)av_x_if_null(av_get_pix_fmt_name(last_format), "none"),
             last_serial, frame->width, frame->height,
             (const char*)av_x_if_null(av_get_pix_fmt_name(frame->format), "none"),
             is->viddec.pkt_serial);
      avfilter_graph_free(&graph);
      graph = avfilter_graph_alloc();
      if ((ret = configure_video_filters(
               graph, is, vfilters_list ? vfilters_list[is->vfilter_idx] : NULL, frame)) < 0) {
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
      last_format = frame->format;
      last_serial = is->viddec.pkt_serial;
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
        if (ret == AVERROR_EOF)
          is->viddec.finished = is->viddec.pkt_serial;
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
                          is->viddec.pkt_serial);
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

int subtitle_thread(void* arg) {
  VideoState* is = arg;
  Frame* sp;
  int got_subtitle;
  double pts;

  for (;;) {
    if (!(sp = frame_queue_peek_writable(&is->subpq)))
      return 0;

    if ((got_subtitle = decoder_decode_frame(&is->subdec, NULL, &sp->sub)) < 0)
      break;

    pts = 0;

    if (got_subtitle && sp->sub.format == 0) {
      if (sp->sub.pts != AV_NOPTS_VALUE)
        pts = sp->sub.pts / (double)AV_TIME_BASE;
      sp->pts = pts;
      sp->serial = is->subdec.pkt_serial;
      sp->width = is->subdec.avctx->width;
      sp->height = is->subdec.avctx->height;
      sp->uploaded = 0;

      /* now we can update the picture count */
      frame_queue_push(&is->subpq);
    } else if (got_subtitle) {
      avsubtitle_free(&sp->sub);
    }
  }
  return 0;
}
