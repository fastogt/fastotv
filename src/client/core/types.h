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

#include <stdint.h>  // for int64_t

#include <common/types.h>  // for time64_t

#include "client_server_types.h"  // for bandwidth_t

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

enum HWAccelID {
  HWACCEL_NONE = 0,
  HWACCEL_AUTO,
  HWACCEL_VDPAU,
  HWACCEL_DXVA2,
  HWACCEL_VDA,
  HWACCEL_VIDEOTOOLBOX,
  HWACCEL_QSV,
  HWACCEL_VAAPI,
  HWACCEL_CUVID
};

typedef common::time64_t msec_t;
typedef common::time64_t clock64_t;
clock64_t invalid_clock();

bandwidth_t CalculateBandwidth(size_t total_downloaded_bytes, msec_t data_interval);

bool IsValidClock(clock64_t clock);
clock64_t GetRealClockTime();  // msec

msec_t ClockToMsec(clock64_t clock);
msec_t GetCurrentMsec();

typedef clock64_t pts_t;
pts_t invalid_pts();
bool IsValidPts(pts_t pts);

enum AvSyncType {
  AV_SYNC_AUDIO_MASTER, /* default choice */
  AV_SYNC_VIDEO_MASTER
};

int64_t get_valid_channel_layout(int64_t channel_layout, int channels);
}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto

namespace common {
std::string ConvertToString(const fasto::fastotv::client::core::HWAccelID& value);
bool ConvertFromString(const std::string& from, fasto::fastotv::client::core::HWAccelID* out);
}  // namespace common
