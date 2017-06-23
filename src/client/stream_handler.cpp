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

#include "client/stream_handler.h"

#include "client/core/application/sdl2_application.h"
#include "client/core/events/events.h"

namespace fasto {
namespace fastotv {
namespace client {

StreamHandler::~StreamHandler() {}

void StreamHandler::HandleFrameResize(core::VideoState* stream,
                                      int width,
                                      int height,
                                      int av_pixel_format,
                                      AVRational sar) {
  core::events::RequestVideoEvent* qevent =
      new core::events::RequestVideoEvent(stream, core::events::FrameInfo(stream, width, height, av_pixel_format, sar));
  fApp->PostEvent(qevent);
}

void StreamHandler::HandleQuitStream(core::VideoState* stream, int exit_code, common::Error err) {
  core::events::QuitStreamEvent* qevent =
      new core::events::QuitStreamEvent(stream, core::events::QuitStreamInfo(stream, exit_code, err));
  fApp->PostEvent(qevent);
}

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
