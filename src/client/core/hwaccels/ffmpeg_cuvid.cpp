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

#include <errno.h>   // for ENOMEM
#include <stddef.h>  // for NULL

extern "C" {
#include <libavcodec/avcodec.h>   // for AVCodecContext
#include <libavutil/buffer.h>     // for av_buffer_unref, AVBufferRef
#include <libavutil/error.h>      // for AVERROR
#include <libavutil/hwcontext.h>  // for AVHWFramesContext, av_hwdev...
#include <libavutil/pixfmt.h>     // for AVPixelFormat::AV_PIX_FMT_CUDA
}

#include <common/logger.h>  // for COMPACT_LOG_ERROR, ERROR_LOG

#include "client/core/ffmpeg_internal.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

static void cuvid_uninit(AVCodecContext* avctx) {
  InputStream* ist = static_cast<InputStream*>(avctx->opaque);
  av_buffer_unref(&ist->hw_frames_ctx);
}

int cuvid_init(AVCodecContext* avctx) {
  InputStream* ist = static_cast<InputStream*>(avctx->opaque);
  AVHWFramesContext* frames_ctx;
  int ret;

  if (!hw_device_ctx) {
    ret = av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, ist->hwaccel_device, NULL, 0);
    if (ret < 0) {
      ERROR_LOG() << "Error creating a CUDA device";
      return ret;
    }
  }

  av_buffer_unref(&ist->hw_frames_ctx);
  ist->hw_frames_ctx = av_hwframe_ctx_alloc(hw_device_ctx);
  if (!ist->hw_frames_ctx) {
    ERROR_LOG() << "Error creating a CUDA frames context";
    return AVERROR(ENOMEM);
  }

  frames_ctx = (AVHWFramesContext*)ist->hw_frames_ctx->data;

  frames_ctx->format = AV_PIX_FMT_CUDA;
  frames_ctx->sw_format = avctx->sw_pix_fmt;
  frames_ctx->width = avctx->width;
  frames_ctx->height = avctx->height;

  ret = av_hwframe_ctx_init(ist->hw_frames_ctx);
  if (ret < 0) {
    ERROR_LOG() << "Error initializing a CUDA frame pool";
    return ret;
  }

  ist->hwaccel_uninit = cuvid_uninit;

  INFO_LOG() << "Using CUDA to decode input stream.";
  return 0;
}
}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
