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

#include "client/player_options.h"

namespace fasto {
namespace fastotv {
namespace client {

PlayerOptions::PlayerOptions()
    : exit_on_keydown(false),
      exit_on_mousedown(false),
      is_full_screen(false),
      default_size(width, height),
      screen_size(0, 0),
      audio_volume(volume) {
}

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
