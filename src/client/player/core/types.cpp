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

#include "client/player/core/types.h"

#include <algorithm>

extern "C" {
#include <libavutil/avutil.h>          // for AV_NOPTS_VALUE
#include <libavutil/channel_layout.h>  // for av_get_channel_layout_nb_channels
}

#include <common/convert2string.h>
#include <common/sprintf.h>
#include <common/time.h>  // for current_mstime

#include "ffmpeg_internal.h"

namespace fastotv {
namespace client {
namespace player {
namespace core {

Size::Size() : width(0), height(0) {}

Size::Size(int width, int height) : width(width), height(height) {}

bool Size::IsValid() const {
  return width != 0 && height != 0;
}

bandwidth_t CalculateBandwidth(size_t total_downloaded_bytes, msec_t data_interval) {
  if (data_interval == 0) {
    return 0;
  }

  bandwidth_t bytes_per_msec = total_downloaded_bytes / data_interval;
  return bytes_per_msec * 1000;
}

clock64_t invalid_clock() {
  return -1;
}

bool IsValidClock(clock64_t clock) {
  return clock != invalid_clock();
}

clock64_t GetRealClockTime() {
  return GetCurrentMsec();
}

msec_t ClockToMsec(clock64_t clock) {
  return clock;
}

msec_t GetCurrentMsec() {
  return common::time::current_mstime();
}

int64_t get_valid_channel_layout(int64_t channel_layout, int channels) {
  if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels) {
    return channel_layout;
  }
  return 0;
}

pts_t invalid_pts() {
  return AV_NOPTS_VALUE;
}

bool IsValidPts(pts_t pts) {
  return pts != invalid_pts();
}

}  // namespace core
}  // namespace player
}  // namespace client
}  // namespace fastotv

namespace common {
std::string ConvertToString(const fastotv::client::player::core::HWAccelID& value) {
  if (value == fastotv::client::player::core::HWACCEL_AUTO) {
    return "auto";
  } else if (value == fastotv::client::player::core::HWACCEL_NONE) {
    return "none";
  }

  for (size_t i = 0; i < fastotv::client::player::core::hwaccel_count(); i++) {
    if (value == fastotv::client::player::core::hwaccels[i].id) {
      return fastotv::client::player::core::hwaccels[i].name;
    }
  }

  return std::string();
}

bool ConvertFromString(const std::string& from, fastotv::client::player::core::HWAccelID* out) {
  if (from.empty() || !out) {
    return false;
  }

  std::string from_copy = from;
  std::transform(from_copy.begin(), from_copy.end(), from_copy.begin(), ::tolower);
  if (from_copy == "auto") {
    *out = fastotv::client::player::core::HWACCEL_AUTO;
    return true;
  } else if (from_copy == "none") {
    *out = fastotv::client::player::core::HWACCEL_NONE;
    return true;
  } else {
    for (size_t i = 0; i < fastotv::client::player::core::hwaccel_count(); i++) {
      if (strcmp(fastotv::client::player::core::hwaccels[i].name, from.c_str()) == 0) {
        *out = fastotv::client::player::core::hwaccels[i].id;
        return true;
      }
    }
    return false;
  }
}

std::string ConvertToString(const fastotv::client::player::core::Size& value) {
  return MemSPrintf("%dx%d", value.width, value.height);
}

bool ConvertFromString(const std::string& from, fastotv::client::player::core::Size* out) {
  if (!out) {
    return false;
  }

  fastotv::client::player::core::Size res;
  size_t del = from.find_first_of('x');
  if (del != std::string::npos) {
    int lwidth;
    if (!ConvertFromString(from.substr(0, del), &lwidth)) {
      return false;
    }
    res.width = lwidth;

    int lheight;
    if (!ConvertFromString(from.substr(del + 1), &lheight)) {
      return false;
    }
    res.height = lheight;
  }

  *out = res;
  return true;
}

}  // namespace common
