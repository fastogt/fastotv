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

typedef uint32_t stream_foramat_t;
enum StreamFmt : stream_foramat_t {
  UNKNOWN_STREAM = 0,
  HAVE_AUDIO_STREAM = (1 << 0),
  HAVE_VIDEO_STREAM = (1 << 1)
};

struct Stats {
  Stats();

  clock_t GetDiffStreams() const;
  double GetFps() const;

  uintmax_t frame_drops_early;
  uintmax_t frame_drops_late;
  uintmax_t frame_processed;

  clock_t master_clock;
  clock_t audio_clock;
  clock_t video_clock;
  stream_foramat_t fmt;

  int audio_queue_size;
  int video_queue_size;

  bandwidth_t video_bandwidth;
  bandwidth_t audio_bandwidth;

 private:
  const common::time64_t start_ts_;
};

std::string ConvertStreamFormatToString(stream_foramat_t fmt);

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
