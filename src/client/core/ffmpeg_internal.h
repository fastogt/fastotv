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

#include "ffmpeg_config.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

extern AVBufferRef* hw_device_ctx;

typedef void hw_uninit_callback_t(AVCodecContext* s);
typedef int hw_get_buffer_callback_t(AVCodecContext* s, AVFrame* frame, int flags);
typedef int hw_retrieve_data_callback_t(AVCodecContext* s, AVFrame* frame);

enum HWAccelID {
  HWACCEL_NONE = 0,
  HWACCEL_AUTO,
  HWACCEL_VDPAU,
  HWACCEL_DXVA2,
  HWACCEL_VDA,
  HWACCEL_VIDEOTOOLBOX,
  HWACCEL_QSV,
  HWACCEL_VAAPI,
  HWACCEL_CUVID,
};

typedef struct InputStream {
  void* hwaccel_ctx;
  hw_uninit_callback_t* hwaccel_uninit;
  hw_get_buffer_callback_t* hwaccel_get_buffer;
  hw_retrieve_data_callback_t* hwaccel_retrieve_data;
  char* hwaccel_device;
  enum AVPixelFormat hwaccel_pix_fmt;
  enum HWAccelID active_hwaccel_id;
  enum HWAccelID hwaccel_id;
  AVBufferRef* hw_frames_ctx;
  enum AVPixelFormat hwaccel_output_format;
} InputStream;

typedef struct HWAccel {
  const char* name;
  int (*init)(AVCodecContext* s);
  enum HWAccelID id;
  enum AVPixelFormat pix_fmt;
} HWAccel;

extern const HWAccel hwaccels[];

size_t hwaccel_count();
const HWAccel* get_hwaccel(enum AVPixelFormat pix_fmt);
