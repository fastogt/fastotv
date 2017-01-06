#include "core/types.h"

#include "cmdutils.h"

#if CONFIG_AVFILTER
int configure_filtergraph(AVFilterGraph* graph,
                                 const char* filtergraph,
                                 AVFilterContext* source_ctx,
                                 AVFilterContext* sink_ctx) {
  int ret, i;
  int nb_filters = graph->nb_filters;
  AVFilterInOut *outputs = NULL, *inputs = NULL;

  if (filtergraph) {
    outputs = avfilter_inout_alloc();
    inputs = avfilter_inout_alloc();
    if (!outputs || !inputs) {
      ret = AVERROR(ENOMEM);
      goto fail;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = source_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = sink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if ((ret = avfilter_graph_parse_ptr(graph, filtergraph, &inputs, &outputs, NULL)) < 0)
      goto fail;
  } else {
    if ((ret = avfilter_link(source_ctx, 0, sink_ctx, 0)) < 0)
      goto fail;
  }

  /* Reorder the filters to ensure that inputs of the custom filters are merged first */
  for (i = 0; i < graph->nb_filters - nb_filters; i++)
    FFSWAP(AVFilterContext*, graph->filters[i], graph->filters[i + nb_filters]);

  ret = avfilter_graph_config(graph, NULL);
fail:
  avfilter_inout_free(&outputs);
  avfilter_inout_free(&inputs);
  return ret;
}

int configure_video_filters(AVFilterGraph* graph,
                                   VideoState* is,
                                   const char* vfilters,
                                   AVFrame* frame) {
  static const enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_BGRA,
                                                AV_PIX_FMT_NONE};
  char sws_flags_str[512] = "";
  char buffersrc_args[256];
  int ret;
  AVFilterContext *filt_src = NULL, *filt_out = NULL, *last_filter = NULL;
  AVCodecParameters* codecpar = is->video_st->codecpar;
  AVRational fr = av_guess_frame_rate(is->ic, is->video_st, NULL);
  AVDictionaryEntry* e = NULL;

  while ((e = av_dict_get(sws_dict, "", e, AV_DICT_IGNORE_SUFFIX))) {
    if (!strcmp(e->key, "sws_flags")) {
      av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", "flags", e->value);
    } else
      av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", e->key, e->value);
  }
  if (strlen(sws_flags_str))
    sws_flags_str[strlen(sws_flags_str) - 1] = '\0';

  graph->scale_sws_opts = av_strdup(sws_flags_str);

  snprintf(buffersrc_args, sizeof(buffersrc_args),
           "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d", frame->width,
           frame->height, frame->format, is->video_st->time_base.num, is->video_st->time_base.den,
           codecpar->sample_aspect_ratio.num, FFMAX(codecpar->sample_aspect_ratio.den, 1));
  if (fr.num && fr.den)
    av_strlcatf(buffersrc_args, sizeof(buffersrc_args), ":frame_rate=%d/%d", fr.num, fr.den);

  if ((ret = avfilter_graph_create_filter(&filt_src, avfilter_get_by_name("buffer"),
                                          "ffplay_buffer", buffersrc_args, NULL, graph)) < 0)
    goto fail;

  ret = avfilter_graph_create_filter(&filt_out, avfilter_get_by_name("buffersink"),
                                     "ffplay_buffersink", NULL, NULL, graph);
  if (ret < 0)
    goto fail;

  if ((ret = av_opt_set_int_list(filt_out, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE,
                                 AV_OPT_SEARCH_CHILDREN)) < 0)
    goto fail;

  last_filter = filt_out;

/* Note: this macro adds a filter before the lastly added filter, so the
 * processing order of the filters is in reverse */
#define INSERT_FILT(name, arg)                                                                     \
  do {                                                                                             \
    AVFilterContext* filt_ctx;                                                                     \
                                                                                                   \
    ret = avfilter_graph_create_filter(&filt_ctx, avfilter_get_by_name(name), "ffplay_" name, arg, \
                                       NULL, graph);                                               \
    if (ret < 0)                                                                                   \
      goto fail;                                                                                   \
                                                                                                   \
    ret = avfilter_link(filt_ctx, 0, last_filter, 0);                                              \
    if (ret < 0)                                                                                   \
      goto fail;                                                                                   \
                                                                                                   \
    last_filter = filt_ctx;                                                                        \
  } while (0)

  if (autorotate) {
    double theta = get_rotation(is->video_st);

    if (fabs(theta - 90) < 1.0) {
      INSERT_FILT("transpose", "clock");
    } else if (fabs(theta - 180) < 1.0) {
      INSERT_FILT("hflip", NULL);
      INSERT_FILT("vflip", NULL);
    } else if (fabs(theta - 270) < 1.0) {
      INSERT_FILT("transpose", "cclock");
    } else if (fabs(theta) > 1.0) {
      char rotate_buf[64];
      snprintf(rotate_buf, sizeof(rotate_buf), "%f*PI/180", theta);
      INSERT_FILT("rotate", rotate_buf);
    }
  }

  if ((ret = configure_filtergraph(graph, vfilters, filt_src, last_filter)) < 0)
    goto fail;

  is->in_video_filter = filt_src;
  is->out_video_filter = filt_out;

fail:
  return ret;
}

int configure_audio_filters(VideoState* is, const char* afilters, int force_output_format) {
  static const enum AVSampleFormat sample_fmts[] = {AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE};
  int sample_rates[2] = {0, -1};
  int64_t channel_layouts[2] = {0, -1};
  int channels[2] = {0, -1};
  AVFilterContext *filt_asrc = NULL, *filt_asink = NULL;
  char aresample_swr_opts[512] = "";
  AVDictionaryEntry* e = NULL;
  char asrc_args[256];
  int ret;

  avfilter_graph_free(&is->agraph);
  if (!(is->agraph = avfilter_graph_alloc()))
    return AVERROR(ENOMEM);

  while ((e = av_dict_get(swr_opts, "", e, AV_DICT_IGNORE_SUFFIX)))
    av_strlcatf(aresample_swr_opts, sizeof(aresample_swr_opts), "%s=%s:", e->key, e->value);
  if (strlen(aresample_swr_opts))
    aresample_swr_opts[strlen(aresample_swr_opts) - 1] = '\0';
  av_opt_set(is->agraph, "aresample_swr_opts", aresample_swr_opts, 0);

  ret = snprintf(asrc_args, sizeof(asrc_args),
                 "sample_rate=%d:sample_fmt=%s:channels=%d:time_base=%d/%d",
                 is->audio_filter_src.freq, av_get_sample_fmt_name(is->audio_filter_src.fmt),
                 is->audio_filter_src.channels, 1, is->audio_filter_src.freq);
  if (is->audio_filter_src.channel_layout)
    snprintf(asrc_args + ret, sizeof(asrc_args) - ret, ":channel_layout=0x%" PRIx64,
             is->audio_filter_src.channel_layout);

  ret = avfilter_graph_create_filter(&filt_asrc, avfilter_get_by_name("abuffer"), "ffplay_abuffer",
                                     asrc_args, NULL, is->agraph);
  if (ret < 0)
    goto end;

  ret = avfilter_graph_create_filter(&filt_asink, avfilter_get_by_name("abuffersink"),
                                     "ffplay_abuffersink", NULL, NULL, is->agraph);
  if (ret < 0)
    goto end;

  if ((ret = av_opt_set_int_list(filt_asink, "sample_fmts", sample_fmts, AV_SAMPLE_FMT_NONE,
                                 AV_OPT_SEARCH_CHILDREN)) < 0)
    goto end;
  if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN)) < 0)
    goto end;

  if (force_output_format) {
    channel_layouts[0] = is->audio_tgt.channel_layout;
    channels[0] = is->audio_tgt.channels;
    sample_rates[0] = is->audio_tgt.freq;
    if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 0, AV_OPT_SEARCH_CHILDREN)) < 0)
      goto end;
    if ((ret = av_opt_set_int_list(filt_asink, "channel_layouts", channel_layouts, -1,
                                   AV_OPT_SEARCH_CHILDREN)) < 0)
      goto end;
    if ((ret = av_opt_set_int_list(filt_asink, "channel_counts", channels, -1,
                                   AV_OPT_SEARCH_CHILDREN)) < 0)
      goto end;
    if ((ret = av_opt_set_int_list(filt_asink, "sample_rates", sample_rates, -1,
                                   AV_OPT_SEARCH_CHILDREN)) < 0)
      goto end;
  }

  if ((ret = configure_filtergraph(is->agraph, afilters, filt_asrc, filt_asink)) < 0)
    goto end;

  is->in_audio_filter = filt_asrc;
  is->out_audio_filter = filt_asink;

end:
  if (ret < 0)
    avfilter_graph_free(&is->agraph);
  return ret;
}
#endif /* CONFIG_AVFILTER */

int64_t get_valid_channel_layout(int64_t channel_layout, int channels) {
  if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels)
    return channel_layout;
  else
    return 0;
}

void set_clock_at(Clock* c, double pts, int serial, double time) {
  c->pts = pts;
  c->last_updated = time;
  c->pts_drift = c->pts - time;
  c->serial = serial;
}

void set_clock(Clock* c, double pts, int serial) {
  double time = av_gettime_relative() / 1000000.0;
  set_clock_at(c, pts, serial, time);
}

double get_clock(Clock* c) {
  if (*c->queue_serial != c->serial)
    return NAN;
  if (c->paused) {
    return c->pts;
  } else {
    double time = av_gettime_relative() / 1000000.0;
    return c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
  }
}


int get_master_sync_type(VideoState* is) {
  if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
    if (is->video_st)
      return AV_SYNC_VIDEO_MASTER;
    else
      return AV_SYNC_AUDIO_MASTER;
  } else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
    if (is->audio_st)
      return AV_SYNC_AUDIO_MASTER;
    else
      return AV_SYNC_EXTERNAL_CLOCK;
  } else {
    return AV_SYNC_EXTERNAL_CLOCK;
  }
}

double compute_target_delay(double delay, VideoState* is) {
  double sync_threshold, diff = 0;

  /* update delay to follow master synchronisation source */
  if (get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER) {
    /* if video is slave, we try to correct big delays by
       duplicating or deleting a frame */
    diff = get_clock(&is->vidclk) - get_master_clock(is);

    /* skip or repeat frame. We take into account the
       delay to compute the threshold. I still don't know
       if it is the best guess */
    sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
    if (!isnan(diff) && fabs(diff) < is->max_frame_duration) {
      if (diff <= -sync_threshold)
        delay = FFMAX(0, delay + diff);
      else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
        delay = delay + diff;
      else if (diff >= sync_threshold)
        delay = 2 * delay;
    }
  }

  av_log(NULL, AV_LOG_TRACE, "video: delay=%0.3f A-V=%f\n", delay, -diff);

  return delay;
}

/* get the current master clock value */
double get_master_clock(VideoState* is) {
  double val;

  switch (get_master_sync_type(is)) {
    case AV_SYNC_VIDEO_MASTER:
      val = get_clock(&is->vidclk);
      break;
    case AV_SYNC_AUDIO_MASTER:
      val = get_clock(&is->audclk);
      break;
    default:
      val = get_clock(&is->extclk);
      break;
  }
  return val;
}
