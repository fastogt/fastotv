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

#include "client/player/media/frames/video_frame.h"

namespace fastotv {
namespace client {
namespace player {
namespace core {
namespace frames {

VideoFrame::VideoFrame() : BaseFrame(), width(0), height(0), format(AV_PIX_FMT_NONE), sar{0, 0} {}

clock64_t CalcDurationBetweenVideoFrames(VideoFrame* vp, VideoFrame* nextvp, clock64_t max_frame_duration) {
  clock64_t duration = nextvp->pts - vp->pts;
  if (!IsValidClock(duration) || duration <= 0 || duration > max_frame_duration) {
    return vp->duration;
  }
  return duration;
}

}  // namespace frames
}  // namespace core
}  // namespace player
}  // namespace client
}  // namespace fastotv
