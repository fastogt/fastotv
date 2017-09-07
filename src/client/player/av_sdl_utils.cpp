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

SDL_Rect CalculateDisplayRect(int scr_xleft,
                              int scr_ytop,
                              int scr_width,
                              int scr_height,
                              int pic_width,
                              int pic_height,
                              AVRational pic_sar) {
  float aspect_ratio;

  if (pic_sar.num == 0) {
    aspect_ratio = 0;
  } else {
    aspect_ratio = av_q2d(pic_sar);
  }

  if (aspect_ratio <= 0.0) {
    aspect_ratio = 1.0;
  }
  aspect_ratio *= static_cast<float>(pic_width) / static_cast<float>(pic_height);

  /* XXX: we suppose the screen has a 1.0 pixel ratio */
  int height = scr_height;
  int width = lrint(height * aspect_ratio) & ~1;
  if (width > scr_width) {
    width = scr_width;
    height = lrint(width / aspect_ratio) & ~1;
  }

  int x = (scr_width - width) / 2;
  int y = (scr_height - height) / 2;
  return {scr_xleft + x, scr_ytop + y, FFMAX(width, 1), FFMAX(height, 1)};
}

common::Error UploadTexture(SDL_Texture* tex, const AVFrame* frame) {
  if (frame->format == AV_PIX_FMT_YUV420P) {
    if (frame->linesize[0] < 0 || frame->linesize[1] < 0 || frame->linesize[2] < 0) {
      return common::make_error_value("Negative linesize is not supported for YUV.", common::ERROR_TYPE);
    }
    if (SDL_UpdateYUVTexture(tex, NULL, frame->data[0], frame->linesize[0], frame->data[1], frame->linesize[1],
                             frame->data[2], frame->linesize[2]) != 0) {
      return common::make_error_value(common::MemSPrintf("UpdateYUVTexture error: %s.", SDL_GetError()),
                                      common::ERROR_TYPE);
    }
    return common::Error();
  } else if (frame->format == AV_PIX_FMT_BGRA) {
    if (frame->linesize[0] < 0) {
      if (SDL_UpdateTexture(tex, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1),
                            -frame->linesize[0]) != 0) {
        return common::make_error_value(common::MemSPrintf("UpdateTexture error: %s.", SDL_GetError()),
                                        common::ERROR_TYPE);
      }
      return common::Error();
    }
    if (SDL_UpdateTexture(tex, NULL, frame->data[0], frame->linesize[0]) != 0) {
      return common::make_error_value(common::MemSPrintf("UpdateTexture error: %s.", SDL_GetError()),
                                      common::ERROR_TYPE);
    }

    return common::Error();
  }

  DNOTREACHED();
  return common::make_error_value(common::MemSPrintf("Unsupported pixel format %d.", frame->format),
                                  common::ERROR_TYPE);
}

}  // namespace player
}  // namespace client
}  // namespace fastotv
