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

#include <string>  // for string

#include "ffmpeg_config.h"

extern "C" {
#include <libavcodec/avcodec.h>  // for AVCodecContext
#include <libavutil/buffer.h>    // for AVBufferRef
#include <libavutil/frame.h>     // for AVFrame
#include <libavutil/pixfmt.h>    // for AVPixelFormat
}

#include "client/player/media/types.h"

typedef void hw_uninit_callback_t(AVCodecContext* s);
typedef int hw_get_buffer_callback_t(AVCodecContext* s, AVFrame* frame, int flags);
typedef int hw_retrieve_data_callback_t(AVCodecContext* s, AVFrame* frame);

namespace fastotv {
namespace client {
namespace player {
namespace core {

extern AVBufferRef* hw_device_ctx;

struct InputStream {
  void* hwaccel_ctx;
  hw_uninit_callback_t* hwaccel_uninit;
  hw_get_buffer_callback_t* hwaccel_get_buffer;
  hw_retrieve_data_callback_t* hwaccel_retrieve_data;
  char* hwaccel_device;
  AVPixelFormat hwaccel_pix_fmt;
  HWAccelID active_hwaccel_id;
  HWAccelID hwaccel_id;
  AVBufferRef* hw_frames_ctx;
  AVPixelFormat hwaccel_output_format;
  AVPixelFormat hwaccel_retrieved_pix_fmt;
};

struct HWAccel {
  const char* name;
  int (*init)(AVCodecContext* s);
  void (*uninit)(AVCodecContext* s);
  HWAccelID id;
  AVPixelFormat pix_fmt;
};

extern const HWAccel hwaccels[];

size_t hwaccel_count();
const HWAccel* get_hwaccel(enum AVPixelFormat pix_fmt);

}  // namespace core
}  // namespace player
}  // namespace client
}  // namespace fastotv
