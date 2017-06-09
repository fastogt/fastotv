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

#include "client/core/video_frame.h"

#include <stddef.h>  // for NULL

#include <SDL2/SDL_render.h>  // for SDL_DestroyTexture

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

VideoFrame::VideoFrame()
    : frame(av_frame_alloc()),
      pts(0),
      duration(0),
      pos(0),
      bmp(NULL),
      allocated(false),
      width(0),
      height(0),
      format(0),
      sar{0, 0},
      uploaded(false),
      flip_v(false) {}

VideoFrame::~VideoFrame() {
  ClearFrame();
  av_frame_free(&frame);
  if (bmp) {
    SDL_DestroyTexture(bmp);
    bmp = NULL;
  }
}

void VideoFrame::ClearFrame() {
  av_frame_unref(frame);
}

clock_t CalcDurationBetweenVideoFrames(VideoFrame* vp, VideoFrame* nextvp, clock_t max_frame_duration) {
  clock_t duration = nextvp->pts - vp->pts;
  if (!IsValidClock(duration) || duration <= 0 || duration > max_frame_duration) {
    return vp->duration;
  }
  return duration;
}

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
