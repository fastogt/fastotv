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

#pragma once

#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_rect.h>

extern "C" {
#include <libavutil/rational.h>  // for AVRational
}

/* Minimum SDL audio buffer size, in samples. */
#define AUDIO_MIN_BUFFER_SIZE 512

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

struct AudioParams;

bool init_audio_params(int64_t wanted_channel_layout, int freq, int channels, AudioParams* audio_hw_params);

struct AudioParams;
void calculate_display_rect(SDL_Rect* rect,
                            int scr_xleft,
                            int scr_ytop,
                            int scr_width,
                            int scr_height,
                            int pic_width,
                            int pic_height,
                            AVRational pic_sar);
int audio_open(void* opaque,
               int64_t wanted_channel_layout,
               int wanted_nb_channels,
               int wanted_sample_rate,
               AudioParams* audio_hw_params,
               SDL_AudioCallback cb);
}
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
