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

#include "client/player/core/frames/base_frame.h"

namespace fastotv {
namespace client {
namespace player {
namespace core {
namespace frames {

/* Common struct for handling all types of decoded data and allocated render buffers. */
struct VideoFrame : public BaseFrame {
  VideoFrame();

  int width;
  int height;
  AVPixelFormat format;  // pixel format in mostly AV_PIX_FMT_YUV420P
  AVRational sar;        // aspect ratio

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoFrame);
};

clock64_t CalcDurationBetweenVideoFrames(VideoFrame* vp, VideoFrame* nextvp, clock64_t max_frame_duration);

}  // namespace frames
}  // namespace core
}  // namespace player
}  // namespace client
}  // namespace fastotv
