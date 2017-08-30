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

#include "client/player/stream_handler.h"

#include "client/player/gui/sdl2_application.h"
#include "client/player/gui/stream_events.h"

namespace fastotv {
namespace client {
namespace player {

StreamHandler::~StreamHandler() {}

void StreamHandler::HandleFrameResize(media::VideoState* stream,
                                      int width,
                                      int height,
                                      int av_pixel_format,
                                      AVRational aspect_ratio) {
  gui::events::RequestVideoEvent* qevent = new gui::events::RequestVideoEvent(
      stream, gui::events::FrameInfo(stream, width, height, av_pixel_format, aspect_ratio));
  fApp->PostEvent(qevent);
}

void StreamHandler::HandleQuitStream(media::VideoState* stream, int exit_code, common::Error err) {
  gui::events::QuitStreamEvent* qevent =
      new gui::events::QuitStreamEvent(stream, gui::events::QuitStreamInfo(stream, exit_code));
  if (err && err->IsError()) {
    fApp->PostEvent(common::make_exception_event(qevent, err));
    return;
  }

  fApp->PostEvent(qevent);
}

}  // namespace player
}  // namespace client
}  // namespace fastotv
