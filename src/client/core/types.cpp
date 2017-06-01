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

#include "client/core/types.h"

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/time.h>
}

#include <common/macros.h>

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

bandwidth_t CalculateBandwidth(size_t total_downloaded_bytes, msec_t data_interval) {
  if (data_interval == 0) {
    return 0;
  }

  bandwidth_t bytes_per_msec = total_downloaded_bytes / data_interval;
  return bytes_per_msec * 1000;
}

clock_t invalid_clock() {
  return -1;
}

bool IsValidClock(clock_t clock) {
  return clock != invalid_clock();
}

clock_t GetRealClockTime() {
  return GetCurrentMsec();
}

msec_t ClockToMsec(clock_t clock) {
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
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
