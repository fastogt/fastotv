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

#include "client/core/events/stream_events.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
namespace events {

StreamInfo::StreamInfo(VideoState* stream) : stream_(stream) {
}

FrameInfo::FrameInfo(VideoState* stream, core::VideoFrame* frame)
    : StreamInfo(stream), frame(frame) {
}

QuitStreamInfo::QuitStreamInfo(VideoState* stream, int exit_code, common::Error err)
    : StreamInfo(stream), exit_code(exit_code), error(err) {
}
}
}
}
}
}
