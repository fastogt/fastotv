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

#include "client/player/core/events/lirc_events.h"

#include <common/macros.h>  // for SIZEOFMASS

namespace {

const std::string g_lirc_types[] = {"LIRC_KEY_OK",   "LIRC_KEY_LEFT", "LIRC_KEY_UP",  "LIRC_KEY_RIGHT",
                                    "LIRC_KEY_DOWN", "LIRC_KEY_EXIT", "LIRC_KEY_MUTE"};
}

namespace common {
std::string ConvertToString(LircCode value) {
  return g_lirc_types[value];
}

bool ConvertFromString(const std::string& from, LircCode* out) {
  if (!out) {
    return false;
  }

  for (uint32_t i = 0; i < SIZEOFMASS(g_lirc_types); ++i) {
    if (from == g_lirc_types[i]) {
      *out = static_cast<LircCode>(i);
      return true;
    }
  }

  return false;
}
}  // namespace common

namespace fastotv {
namespace client {
namespace player {
namespace core {
namespace events {}
}  // namespace core
}  // namespace player
}  // namespace client
}  // namespace fastotv
