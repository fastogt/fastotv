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

#include <SDL2/SDL_render.h>  // for SDL_Texture

extern "C" {
#include <libavutil/frame.h>     // for AVFrame
#include <libavutil/rational.h>  // for AVRational
}

#include <common/macros.h>  // for DISALLOW_COPY_AND_ASSIGN

#include "client/core/types.h"  // for clock64_t

extern "C" {
#include <libavutil/frame.h>     // for AVFrame
#include <libavutil/rational.h>  // for AVRational
}

#include <common/macros.h>  // for DISALLOW_COPY_AND_ASSIGN

#include "client/core/clock.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

/* Common struct for handling all types of decoded data and allocated render buffers. */
struct VideoFrame {
  VideoFrame();
  ~VideoFrame();
  void ClearFrame();

  AVFrame* frame;
  clock64_t pts;      /* presentation timestamp for the frame */
  clock64_t duration; /* estimated duration of the frame */
  int64_t pos;        /* byte position of the frame in the input file */

  int width;
  int height;
  int format;      // pixel format in mostly AV_PIX_FMT_YUV420P
  AVRational sar;  // aspect ratio

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoFrame);
};

clock64_t CalcDurationBetweenVideoFrames(VideoFrame* vp, VideoFrame* nextvp, clock64_t max_frame_duration);

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
