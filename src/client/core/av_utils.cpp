/*  Copyright (C) 2014-2017 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

#include "client/core/av_utils.h"

extern "C" {
#include <libavutil/display.h>
#include <libavutil/eval.h>
#include <libavutil/opt.h>
}

#include <common/macros.h>

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

int check_stream_specifier(AVFormatContext* s, AVStream* st, const char* spec) {
  int ret = avformat_match_stream_specifier(s, st, spec);
  if (ret < 0) {
    ERROR_LOG() << "Invalid stream specifier: " << spec;
  }
  return ret;
}

double q2d_diff(AVRational a) {
  double div = a.num / static_cast<double>(a.den);
  return div * 1000.0;
}

AVRational guess_sample_aspect_ratio(AVStream* stream, AVFrame* frame) {
  AVRational undef = {0, 1};
  AVRational stream_sample_aspect_ratio = stream ? stream->sample_aspect_ratio : undef;
  AVRational codec_sample_aspect_ratio = stream && stream->codecpar ? stream->codecpar->sample_aspect_ratio : undef;
  AVRational frame_sample_aspect_ratio = frame ? frame->sample_aspect_ratio : codec_sample_aspect_ratio;

  av_reduce(&stream_sample_aspect_ratio.num, &stream_sample_aspect_ratio.den, stream_sample_aspect_ratio.num,
            stream_sample_aspect_ratio.den, INT_MAX);
  if (stream_sample_aspect_ratio.num <= 0 || stream_sample_aspect_ratio.den <= 0) {
    stream_sample_aspect_ratio = undef;
  }

  av_reduce(&frame_sample_aspect_ratio.num, &frame_sample_aspect_ratio.den, frame_sample_aspect_ratio.num,
            frame_sample_aspect_ratio.den, INT_MAX);
  if (frame_sample_aspect_ratio.num <= 0 || frame_sample_aspect_ratio.den <= 0) {
    frame_sample_aspect_ratio = undef;
  }

  if (stream_sample_aspect_ratio.num) {
    return stream_sample_aspect_ratio;
  }

  return frame_sample_aspect_ratio;
}

double get_rotation(AVStream* st) {
  if (!st) {
    DNOTREACHED();
    return 0.0;
  }

  AVDictionaryEntry* rotate_tag = av_dict_get(st->metadata, "rotate", NULL, 0);
  uint8_t* displaymatrix = av_stream_get_side_data(st, AV_PKT_DATA_DISPLAYMATRIX, NULL);
  double theta = 0;

  if (rotate_tag && *rotate_tag->value && strcmp(rotate_tag->value, "0")) {
    char* tail;
    theta = av_strtod(rotate_tag->value, &tail);
    if (*tail) {
      theta = 0;
    }
  }

  if (displaymatrix && !theta) {
    theta = -av_display_rotation_get(reinterpret_cast<int32_t*>(displaymatrix));
  }

  theta -= 360 * floor(theta / 360 + 0.9 / 360);

  if (fabs(theta - 90 * round(theta / 90)) > 2) {
    WARNING_LOG() << "Odd rotation angle.";
  }

  return theta;
}

bool is_realtime(AVFormatContext* s) {
  if (!strcmp(s->iformat->name, "rtp") || !strcmp(s->iformat->name, "rtsp") || !strcmp(s->iformat->name, "sdp")) {
    return true;
  }

  if (s->pb && (!strncmp(s->filename, "rtp:", 4) || !strncmp(s->filename, "udp:", 4))) {
    return true;
  }
  return false;
}

#if CONFIG_AVFILTER
int configure_filtergraph(AVFilterGraph* graph,
                          const char* filtergraph,
                          AVFilterContext* source_ctx,
                          AVFilterContext* sink_ctx) {
  AVFilterInOut *outputs = NULL, *inputs = NULL;
  int ret;
  if (filtergraph) {
    outputs = avfilter_inout_alloc();
    inputs = avfilter_inout_alloc();
    if (!outputs || !inputs) {
      avfilter_inout_free(&outputs);
      avfilter_inout_free(&inputs);
      return AVERROR(ENOMEM);
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = source_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = sink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    ret = avfilter_graph_parse_ptr(graph, filtergraph, &inputs, &outputs, NULL);
    if (ret < 0) {
      avfilter_inout_free(&outputs);
      avfilter_inout_free(&inputs);
      return ret;
    }
  } else {
    ret = avfilter_link(source_ctx, 0, sink_ctx, 0);
    if (ret < 0) {
      avfilter_inout_free(&outputs);
      avfilter_inout_free(&inputs);
      return ret;
    }
  }

  unsigned int nb_filters = graph->nb_filters;
  /* Reorder the filters to ensure that inputs of the custom filters are merged first */
  for (unsigned int i = 0; i < graph->nb_filters - nb_filters; i++) {
    FFSWAP(AVFilterContext*, graph->filters[i], graph->filters[i + nb_filters]);
  }

  ret = avfilter_graph_config(graph, NULL);
  avfilter_inout_free(&outputs);
  avfilter_inout_free(&inputs);
  return ret;
}
#endif

AVDictionary* filter_codec_opts(AVDictionary* opts,
                                enum AVCodecID codec_id,
                                AVFormatContext* s,
                                AVStream* st,
                                AVCodec* codec) {
  AVDictionary* ret = NULL;
  AVDictionaryEntry* t = NULL;
  int flags = s->oformat ? AV_OPT_FLAG_ENCODING_PARAM : AV_OPT_FLAG_DECODING_PARAM;
  char prefix = 0;
  const AVClass* cc = avcodec_get_class();

  if (!codec)
    codec = s->oformat ? avcodec_find_encoder(codec_id) : avcodec_find_decoder(codec_id);

  switch (st->codecpar->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
      prefix = 'v';
      flags |= AV_OPT_FLAG_VIDEO_PARAM;
      break;
    case AVMEDIA_TYPE_AUDIO:
      prefix = 'a';
      flags |= AV_OPT_FLAG_AUDIO_PARAM;
      break;
    case AVMEDIA_TYPE_SUBTITLE:
      prefix = 's';
      flags |= AV_OPT_FLAG_SUBTITLE_PARAM;
      break;
    case AVMEDIA_TYPE_UNKNOWN:
      break;
    case AVMEDIA_TYPE_ATTACHMENT:
      break;
    case AVMEDIA_TYPE_DATA:
      break;
    case AVMEDIA_TYPE_NB:
      break;
  }

  while ((t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX))) {
    char* p = strchr(t->key, ':');

    /* check stream specification in opt name */
    if (p)
      switch (check_stream_specifier(s, st, p + 1)) {
        case 1:
          *p = 0;
          break;
        case 0:
          continue;
        default:
          NOTREACHED();
      }

    if (av_opt_find(&cc, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ) || !codec ||
        (codec->priv_class && av_opt_find(&codec->priv_class, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ))) {
      av_dict_set(&ret, t->key, t->value, 0);
    } else if (t->key[0] == prefix && av_opt_find(&cc, t->key + 1, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ)) {
      av_dict_set(&ret, t->key + 1, t->value, 0);
    }

    if (p) {
      *p = ':';
    }
  }
  return ret;
}

AVDictionary** setup_find_stream_info_opts(AVFormatContext* s, AVDictionary* codec_opts) {
  if (!s->nb_streams) {
    return NULL;
  }

  AVDictionary** opts = static_cast<AVDictionary**>(av_mallocz_array(s->nb_streams, sizeof(*opts)));
  if (!opts) {
    ERROR_LOG() << "Could not alloc memory for stream options.";
    return NULL;
  }

  for (unsigned int i = 0; i < s->nb_streams; i++) {
    opts[i] = filter_codec_opts(codec_opts, s->streams[i]->codecpar->codec_id, s, s->streams[i], NULL);
  }
  return opts;
}

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
