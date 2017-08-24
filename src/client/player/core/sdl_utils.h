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
#include <SDL2/SDL_render.h>  // for SDL_Renderer, SDL_Texture

extern "C" {
#include <libavutil/rational.h>  // for AVRational
}

#include "client/player/core/types.h"

/* Minimum SDL audio buffer size, in samples. */
#define AUDIO_MIN_BUFFER_SIZE 512

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

struct AudioParams;

bool init_audio_params(int64_t wanted_channel_layout, int freq, int channels, AudioParams* audio_hw_params)
    WARN_UNUSED_RESULT;

bool audio_open(void* opaque,
                int64_t wanted_channel_layout,
                int wanted_nb_channels,
                int wanted_sample_rate,
                SDL_AudioCallback cb,
                AudioParams* audio_hw_params,
                int* audio_buff_size) WARN_UNUSED_RESULT;

SDL_Rect calculate_display_rect(int scr_xleft,
                                int scr_ytop,
                                int scr_width,
                                int scr_height,
                                int pic_width,
                                int pic_height,
                                AVRational pic_sar);

bool create_window(Size window_size,
                   bool is_full_screen,
                   const std::string& title,
                   SDL_Renderer** renderer,
                   SDL_Window** window) WARN_UNUSED_RESULT;

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
