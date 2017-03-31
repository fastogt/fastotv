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

#include <libavcodec/avcodec.h>

typedef void hw_uninit_callback_t(AVCodecContext* s);
typedef int hw_get_buffer_callback_t(AVCodecContext* s, AVFrame* frame, int flags);
typedef int hw_retrieve_data_callback_t(AVCodecContext* s, AVFrame* frame);

typedef struct InputStream {
  void* hwaccel_ctx;
  hw_uninit_callback_t* hwaccel_uninit;
  hw_get_buffer_callback_t* hwaccel_get_buffer;
  hw_retrieve_data_callback_t* hwaccel_retrieve_data;
  const char* hwaccel_device;
} InputStream;

int vdpau_init(AVCodecContext* decoder_ctx);
