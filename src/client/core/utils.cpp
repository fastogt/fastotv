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

#include "client/core/utils.h"

#include <math.h>    // for lrint, fabs, floor, round
#include <stdint.h>  // for uint8_t
#include <stdlib.h>  // for atoi, exit
#include <string.h>  // for NULL, strcmp, strncmp, etc
#include <errno.h>   // for ENOMEM

#include <ostream>  // for operator<<, basic_ostream, etc

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>   // for av_log2, FFMAX, FFMIN, etc
#include <libavutil/display.h>  // for av_display_rotation_get
#include <libavutil/error.h>    // for AVERROR
#include <libavutil/eval.h>     // for av_strtod
#include <libavutil/log.h>      // for AVClass
#include <libavutil/mem.h>      // for av_strdup, av_mallocz_array
#include <libavutil/opt.h>      // for av_opt_find, etc
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>  // for sws_getCachedContext, etc
}

#include <common/logger.h>  // for LogMessage, etc

#include "client/core/audio_params.h"  // for AudioParams

/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

namespace {
const unsigned sws_flags = SWS_BICUBIC;
}

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

double q2d_diff(AVRational a) {
  return a.num / (double)a.den * 1000.0;
}

int check_stream_specifier(AVFormatContext* s, AVStream* st, const char* spec) {
  int ret = avformat_match_stream_specifier(s, st, spec);
  if (ret < 0) {
    ERROR_LOG() << "Invalid stream specifier: " << spec;
  }
  return ret;
}

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
          exit_program(1);
      }

    if (av_opt_find(&cc, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ) || !codec ||
        (codec->priv_class &&
         av_opt_find(&codec->priv_class, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ))) {
      av_dict_set(&ret, t->key, t->value, 0);
    } else if (t->key[0] == prefix &&
               av_opt_find(&cc, t->key + 1, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ)) {
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
    opts[i] =
        filter_codec_opts(codec_opts, s->streams[i]->codecpar->codec_id, s, s->streams[i], NULL);
  }
  return opts;
}

double get_rotation(AVStream* st) {
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

void exit_program(int ret) {
  exit(ret);
}

#if CONFIG_AVFILTER
int configure_filtergraph(AVFilterGraph* graph,
                          const std::string& filtergraph,
                          AVFilterContext* source_ctx,
                          AVFilterContext* sink_ctx) {
  AVFilterInOut *outputs = NULL, *inputs = NULL;
  int ret;
  if (!filtergraph.empty()) {
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

    ret = avfilter_graph_parse_ptr(graph, filtergraph.c_str(), &inputs, &outputs, NULL);
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

void calculate_display_rect(SDL_Rect* rect,
                            int scr_xleft,
                            int scr_ytop,
                            int scr_width,
                            int scr_height,
                            int pic_width,
                            int pic_height,
                            AVRational pic_sar) {
  float aspect_ratio;

  if (pic_sar.num == 0) {
    aspect_ratio = 0;
  } else {
    aspect_ratio = av_q2d(pic_sar);
  }

  if (aspect_ratio <= 0.0) {
    aspect_ratio = 1.0;
  }
  aspect_ratio *= static_cast<float>(pic_width) / static_cast<float>(pic_height);

  /* XXX: we suppose the screen has a 1.0 pixel ratio */
  int height = scr_height;
  int width = lrint(height * aspect_ratio) & ~1;
  if (width > scr_width) {
    width = scr_width;
    height = lrint(width / aspect_ratio) & ~1;
  }
  int x = (scr_width - width) / 2;
  int y = (scr_height - height) / 2;
  rect->x = scr_xleft + x;
  rect->y = scr_ytop + y;
  rect->w = FFMAX(width, 1);
  rect->h = FFMAX(height, 1);
}

int upload_texture(SDL_Texture* tex, const AVFrame* frame) {
  int ret = 0;
  switch (frame->format) {
    case AV_PIX_FMT_YUV420P:
      if (frame->linesize[0] < 0 || frame->linesize[1] < 0 || frame->linesize[2] < 0) {
        ERROR_LOG() << "Negative linesize is not supported for YUV.";
        return -1;
      }
      ret = SDL_UpdateYUVTexture(tex, NULL, frame->data[0], frame->linesize[0], frame->data[1],
                                 frame->linesize[1], frame->data[2], frame->linesize[2]);
      break;
    case AV_PIX_FMT_BGRA:
      if (frame->linesize[0] < 0) {
        ret =
            SDL_UpdateTexture(tex, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1),
                              -frame->linesize[0]);
      } else {
        ret = SDL_UpdateTexture(tex, NULL, frame->data[0], frame->linesize[0]);
      }
      break;
    default:
      break;
  }
  return ret;
}

int audio_open(void* opaque,
               int64_t wanted_channel_layout,
               int wanted_nb_channels,
               int wanted_sample_rate,
               struct AudioParams* audio_hw_params,
               SDL_AudioCallback cb) {
  SDL_AudioSpec wanted_spec, spec;
  static const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
  static const int next_sample_rates[] = {0, 44100, 48000, 96000, 192000};
  int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;

  const char* env = SDL_getenv("SDL_AUDIO_CHANNELS");
  if (env) {
    wanted_nb_channels = atoi(env);
    wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
  }
  if (!wanted_channel_layout ||
      wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
    wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
    wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
  }
  wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
  wanted_spec.channels = wanted_nb_channels;
  wanted_spec.freq = wanted_sample_rate;
  if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
    ERROR_LOG() << "Invalid sample rate or channel count!";
    return -1;
  }
  while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq) {
    next_sample_rate_idx--;
  }
  wanted_spec.format = AUDIO_S16SYS;
  wanted_spec.silence = 0;
  wanted_spec.samples = FFMAX(AUDIO_MIN_BUFFER_SIZE,
                              2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
  wanted_spec.callback = cb;
  wanted_spec.userdata = opaque;
  while (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
    WARNING_LOG() << "SDL_OpenAudio (" << static_cast<int>(wanted_spec.channels) << " channels, "
                  << wanted_spec.freq << " Hz): " << SDL_GetError();
    wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
    if (!wanted_spec.channels) {
      wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
      wanted_spec.channels = wanted_nb_channels;
      if (!wanted_spec.freq) {
        ERROR_LOG() << "No more combinations to try, audio open failed";
        return -1;
      }
    }
    wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
  }
  if (spec.format != AUDIO_S16SYS) {
    ERROR_LOG() << "SDL advised audio format " << spec.format << " is not supported!";
    return -1;
  }
  if (spec.channels != wanted_spec.channels) {
    wanted_channel_layout = av_get_default_channel_layout(spec.channels);
    if (!wanted_channel_layout) {
      ERROR_LOG() << "SDL advised channel count " << spec.channels << " is not supported!";
      return -1;
    }
  }

  struct AudioParams laudio_hw_params;
  if (!init_audio_params(wanted_channel_layout, spec.freq, spec.channels, &laudio_hw_params)) {
    ERROR_LOG() << "Failed to init audio parametrs";
    return -1;
  }

  *audio_hw_params = laudio_hw_params;
  return spec.size;
}

bool init_audio_params(int64_t wanted_channel_layout,
                       int freq,
                       int channels,
                       struct AudioParams* audio_hw_params) {
  if (!audio_hw_params) {
    return false;
  }

  struct AudioParams laudio_hw_params;
  laudio_hw_params.fmt = AV_SAMPLE_FMT_S16;
  laudio_hw_params.freq = freq;
  laudio_hw_params.channel_layout = wanted_channel_layout;
  laudio_hw_params.channels = channels;
  laudio_hw_params.frame_size =
      av_samples_get_buffer_size(NULL, laudio_hw_params.channels, 1, laudio_hw_params.fmt, 1);
  laudio_hw_params.bytes_per_sec = av_samples_get_buffer_size(
      NULL, laudio_hw_params.channels, laudio_hw_params.freq, laudio_hw_params.fmt, 1);
  if (laudio_hw_params.bytes_per_sec <= 0 || laudio_hw_params.frame_size <= 0) {
    return false;
  }

  *audio_hw_params = laudio_hw_params;
  return true;
}

bool is_realtime(AVFormatContext* s) {
  if (!strcmp(s->iformat->name, "rtp") || !strcmp(s->iformat->name, "rtsp") ||
      !strcmp(s->iformat->name, "sdp")) {
    return true;
  }

  if (s->pb && (!strncmp(s->filename, "rtp:", 4) || !strncmp(s->filename, "udp:", 4))) {
    return true;
  }
  return false;
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

}  // namespace core
}
}
}
