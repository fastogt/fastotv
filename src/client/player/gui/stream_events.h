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

extern "C" {
#include <libavutil/rational.h>  // for AVRational
}

#include "client/player/gui/events_base.h"

namespace fastotv {
namespace client {
namespace player {
namespace media {
class VideoState;
}
namespace gui {
namespace events {

struct FrameInfo {
  FrameInfo(media::VideoState* stream, int width, int height, int av_pixel_format, AVRational aspect_ratio);

  media::VideoState* stream_;
  int width;
  int height;
  int av_pixel_format;
  AVRational aspect_ratio;
};

struct QuitStreamInfo {
  QuitStreamInfo(media::VideoState* stream, int exit_code);

  media::VideoState* stream_;
  int exit_code;
};

typedef EventBase<REQUEST_VIDEO_EVENT, FrameInfo> RequestVideoEvent;
typedef EventBase<QUIT_STREAM_EVENT, QuitStreamInfo> QuitStreamEvent;

}  // namespace events
}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
