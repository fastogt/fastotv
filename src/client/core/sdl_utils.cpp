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

#include "client/core/sdl_utils.h"

extern "C" {
#include <libavutil/common.h>
#include <libavutil/channel_layout.h>
}

#include <common/macros.h>

#include "client/core/audio_params.h"  // for AudioParams

/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 10
#define SDL_AUDIO_MIN_BUFFER_SIZE 512

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

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

bool init_audio_params(int64_t wanted_channel_layout, int freq, int channels, AudioParams* audio_hw_params) {
  if (!audio_hw_params) {
    return false;
  }

  AudioParams laudio_hw_params;
  laudio_hw_params.fmt = AV_SAMPLE_FMT_S16;
  laudio_hw_params.freq = freq;
  laudio_hw_params.channel_layout = wanted_channel_layout;
  laudio_hw_params.channels = channels;
  laudio_hw_params.frame_size = av_samples_get_buffer_size(NULL, laudio_hw_params.channels, 1, laudio_hw_params.fmt, 1);
  laudio_hw_params.bytes_per_sec =
      av_samples_get_buffer_size(NULL, laudio_hw_params.channels, laudio_hw_params.freq, laudio_hw_params.fmt, 1);
  if (laudio_hw_params.bytes_per_sec <= 0 || laudio_hw_params.frame_size <= 0) {
    return false;
  }

  *audio_hw_params = laudio_hw_params;
  return true;
}

int audio_open(void* opaque,
               int64_t wanted_channel_layout,
               int wanted_nb_channels,
               int wanted_sample_rate,
               AudioParams* audio_hw_params,
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
  if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
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
  const double samples_per_call = static_cast<double>(wanted_spec.freq) / SDL_AUDIO_MAX_CALLBACKS_PER_SEC;
  const Uint16 audio_buff_size_calc = 2 << av_log2(samples_per_call);
  const Uint16 audio_buff_size = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, audio_buff_size_calc);
  wanted_spec.samples = FFMAX(AUDIO_MIN_BUFFER_SIZE, audio_buff_size);  // Audio buffer size in samples
  wanted_spec.callback = cb;
  wanted_spec.userdata = opaque;
  while (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
    WARNING_LOG() << "SDL_OpenAudio (" << static_cast<int>(wanted_spec.channels) << " channels, " << wanted_spec.freq
                  << " Hz): " << SDL_GetError();
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

  AudioParams laudio_hw_params;
  if (!init_audio_params(wanted_channel_layout, spec.freq, spec.channels, &laudio_hw_params)) {
    ERROR_LOG() << "Failed to init audio parametrs";
    return -1;
  }

  *audio_hw_params = laudio_hw_params;
  return spec.size;
}

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
