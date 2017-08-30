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

#include "client/player/gui/stream_events.h"

namespace fastotv {
namespace client {
namespace player {
namespace gui {
namespace events {

StreamInfo::StreamInfo(media::VideoState* stream) : stream_(stream) {}

FrameInfo::FrameInfo(media::VideoState* stream, int width, int height, int av_pixel_format, AVRational aspect_ratio)
    : StreamInfo(stream), width(width), height(height), av_pixel_format(av_pixel_format), aspect_ratio(aspect_ratio) {}

QuitStreamInfo::QuitStreamInfo(media::VideoState* stream, int exit_code, common::Error err)
    : StreamInfo(stream), exit_code(exit_code), error(err) {}

}  // namespace events
}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
