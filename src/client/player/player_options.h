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

#include "client/player/core/types.h"

namespace fasto {
namespace fastotv {
namespace client {

struct PlayerOptions {
  enum { width = 640, height = 480, volume = 100 };
  PlayerOptions();

  bool exit_on_keydown;
  bool exit_on_mousedown;
  bool is_full_screen;

  core::Size default_size;
  core::Size screen_size;

  audio_volume_t audio_volume;  // Range: 0 - 100
  stream_id last_showed_channel_id;
};

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
