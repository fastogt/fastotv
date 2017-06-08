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

#include "client/core/types.h"  // for clock_t

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

enum StreamFmt { UNKNOWN_STREAM, ONLY_AUDIO_STREAM, ONLY_VIDEO_STREAM, VIDEO_AUDIO_STREAM };

struct Stats {
  Stats();

  clock_t GetDiffStreams() const;

  int frame_drops_early;
  int frame_drops_late;

  clock_t master_clock;
  clock_t audio_clock;
  clock_t video_clock;
  StreamFmt fmt;

  int audio_queue_size;
  int video_queue_size;

  bandwidth_t video_bandwidth;
  bandwidth_t audio_bandwidth;
};

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto

namespace common {
std::string ConvertToString(fasto::fastotv::client::core::StreamFmt value);
bool ConvertFromString(const std::string& from, fasto::fastotv::client::core::StreamFmt* out);
}  // namespace common
