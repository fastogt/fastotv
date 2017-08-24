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

#include "client/player/av_sdl_utils.h"

namespace fastotv {
namespace client {
namespace player {

common::Error UploadTexture(SDL_Texture* tex, const AVFrame* frame) {
  if (frame->format == AV_PIX_FMT_YUV420P) {
    if (frame->linesize[0] < 0 || frame->linesize[1] < 0 || frame->linesize[2] < 0) {
      return common::make_error_value("Negative linesize is not supported for YUV.", common::Value::E_ERROR);
    }
    if (SDL_UpdateYUVTexture(tex, NULL, frame->data[0], frame->linesize[0], frame->data[1], frame->linesize[1],
                             frame->data[2], frame->linesize[2]) != 0) {
      return common::make_error_value(common::MemSPrintf("UpdateYUVTexture error: %s.", SDL_GetError()),
                                      common::Value::E_ERROR);
    }
    return common::Error();
  } else if (frame->format == AV_PIX_FMT_BGRA) {
    if (frame->linesize[0] < 0) {
      if (SDL_UpdateTexture(tex, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1),
                            -frame->linesize[0]) != 0) {
        return common::make_error_value(common::MemSPrintf("UpdateTexture error: %s.", SDL_GetError()),
                                        common::Value::E_ERROR);
      }
      return common::Error();
    }
    if (SDL_UpdateTexture(tex, NULL, frame->data[0], frame->linesize[0]) != 0) {
      return common::make_error_value(common::MemSPrintf("UpdateTexture error: %s.", SDL_GetError()),
                                      common::Value::E_ERROR);
    }

    return common::Error();
  }

  DNOTREACHED();
  return common::make_error_value(common::MemSPrintf("Unsupported pixel format %d.", frame->format),
                                  common::Value::E_ERROR);
}

}  // namespace player
}  // namespace client
}  // namespace fastotv
