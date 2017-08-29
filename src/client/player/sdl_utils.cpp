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

#include "client/player/sdl_utils.h"

#include <SDL2/SDL_audio.h>

#include <common/macros.h>

namespace fastotv {
namespace client {
namespace player {

int ConvertToSDLVolume(int val) {
  val = stable_value_in_range(val, 0, 100);
  return stable_value_in_range(SDL_MIX_MAXVOLUME * val / 100, 0, SDL_MIX_MAXVOLUME);
}

}  // namespace player
}  // namespace client
}  // namespace fastotv
