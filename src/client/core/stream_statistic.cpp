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

#include "client/core/stream_statistic.h"

namespace {
const std::string g_formats_types[] = {"   ", "M-A", "M-V", "A-V"};
}

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

Stats::Stats()
    : frame_drops_early(0),
      frame_drops_late(0),
      master_clock(0),
      audio_clock(0),
      video_clock(0),
      fmt(UNKNOWN_STREAM),
      audio_queue_size(0),
      video_queue_size(0),
      video_bandwidth(0),
      audio_bandwidth(0) {}

clock_t Stats::GetDiffStreams() const {
  if (fmt == VIDEO_AUDIO_STREAM) {
    return audio_clock - video_clock;
  } else if (fmt == ONLY_VIDEO_STREAM) {
    return master_clock - video_clock;
  } else if (fmt == ONLY_AUDIO_STREAM) {
    return master_clock - audio_clock;
  }

  return 0;
}

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto

namespace common {
std::string ConvertToString(fasto::fastotv::client::core::StreamFmt value) {
  return g_formats_types[value];
}

bool ConvertFromString(const std::string& from, fasto::fastotv::client::core::StreamFmt* out) {
  for (uint32_t i = 0; i < SIZEOFMASS(g_formats_types); ++i) {
    if (from == g_formats_types[i]) {
      *out = static_cast<fasto::fastotv::client::core::StreamFmt>(i);
      return true;
    }
  }

  return false;
}

}  // namespace common
