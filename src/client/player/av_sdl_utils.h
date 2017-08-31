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

#include <SDL2/SDL_render.h>  // for SDL_Renderer, SDL_Texture
extern "C" {
#include <libavutil/frame.h>  // for AVFrame
}

#include <common/error.h>  // for Error

namespace fastotv {
namespace client {
namespace player {

SDL_Rect CalculateDisplayRect(int scr_xleft,
                              int scr_ytop,
                              int scr_width,
                              int scr_height,
                              int pic_width,
                              int pic_height,
                              AVRational pic_sar);
common::Error UploadTexture(SDL_Texture* tex, const AVFrame* frame) WARN_UNUSED_RESULT;

}  // namespace player
}  // namespace client
}  // namespace fastotv
