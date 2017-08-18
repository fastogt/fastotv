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

#include "client/core/types.h"  // for clock64_t

extern "C" {
#include <libavutil/rational.h>    // for AVRational
}

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

typedef uint32_t stream_format_t;
enum StreamFmt : stream_format_t { UNKNOWN_STREAM = 0, HAVE_AUDIO_STREAM = (1 << 0), HAVE_VIDEO_STREAM = (1 << 1) };

struct Stats {  // stream realtime statistic
  Stats();

  clock64_t GetDiffStreams() const;  // msec
  double GetFps() const;
  double GetSreamFps() const;

  size_t frame_drops_early;
  size_t frame_drops_late;
  size_t frame_processed;

  clock64_t master_pts;
  clock64_t master_clock;  // msec
  clock64_t audio_clock;   // msec
  clock64_t video_clock;   // msec
  stream_format_t fmt;

  int audio_queue_size;  // bytes
  int video_queue_size;  // bytes

  bandwidth_t video_bandwidth;  // bytes/s
  bandwidth_t audio_bandwidth;  // bytes/s
  HWAccelID active_hwaccel;

  AVRational frame_rate;

 private:
  const common::time64_t start_ts_;
};

std::string ConvertStreamFormatToString(stream_format_t fmt);

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
