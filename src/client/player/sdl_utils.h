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

extern "C" {
#include <libavutil/rational.h>  // for AVRational
}

#include "client/player/media/types.h"

namespace fastotv {
namespace client {
namespace player {

namespace media {
struct AudioParams;
}

int ConvertToSDLVolume(int val);

bool init_audio_params(int64_t wanted_channel_layout, int freq, int channels, media::AudioParams* audio_hw_params)
    WARN_UNUSED_RESULT;

bool audio_open(void* opaque,
                int64_t wanted_channel_layout,
                int wanted_nb_channels,
                int wanted_sample_rate,
                SDL_AudioCallback cb,
                media::AudioParams* audio_hw_params,
                int* audio_buff_size) WARN_UNUSED_RESULT;

}  // namespace player
}  // namespace client
}  // namespace fastotv
