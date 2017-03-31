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

#include <limits>

#include <common/time.h>

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

typedef common::time64_t msec_t;
typedef common::time64_t clock_t;
clock_t invalid_clock();

bool IsValidClock(clock_t clock);
clock_t GetRealClockTime();  // msec

msec_t ClockToMsec(clock_t clock);
msec_t GetCurrentMsec();

typedef int64_t pts_t;
pts_t invalid_pts();
bool IsValidPts(pts_t pts);

enum AvSyncType {
  AV_SYNC_AUDIO_MASTER, /* default choice */
  AV_SYNC_VIDEO_MASTER
};

int64_t get_valid_channel_layout(int64_t channel_layout, int channels);

}
}
}
}
