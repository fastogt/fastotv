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

#include "client/player/media/hwaccels/ffmpeg_vaapi.h"

#include <errno.h>   // for ENOMEM
#include <stddef.h>  // for NULL

extern "C" {
#include <libavutil/buffer.h>     // for av_buffer_unref, AVBufferRef
#include <libavutil/error.h>      // for AVERROR
#include <libavutil/frame.h>      // for av_frame_free, AVFrame, av_...
#include <libavutil/hwcontext.h>  // for AVHWFramesContext, AVHWDevi...
#include <libavutil/mem.h>        // for av_free, av_mallocz
#include <libavutil/pixfmt.h>     // for AVPixelFormat::AV_PIX_FMT_V...
}

#include <common/logger.h>  // for COMPACT_LOG_ERROR, ERROR_LOG
#include <common/macros.h>  // for UNUSED

#include "client/player/media/ffmpeg_internal.h"

#define DEFAULT_SURFACES 20

namespace fastotv {
namespace client {
namespace player {
namespace media {

typedef struct VAAPIDecoderContext {
  AVBufferRef* device_ref;
  AVHWDeviceContext* device;
  AVBufferRef* frames_ref;
  AVHWFramesContext* frames;

  // The output need not have the same format, width and height as the
  // decoded frames - the copy for non-direct-mapped access is actually
  // a whole vpp instance which can do arbitrary scaling and format
  // conversion.
  enum AVPixelFormat output_format;
} VAAPIDecoderContext;

static int vaapi_get_buffer(AVCodecContext* avctx, AVFrame* frame, int flags) {
  UNUSED(flags);
  InputStream* ist = static_cast<InputStream*>(avctx->opaque);
  VAAPIDecoderContext* ctx = static_cast<VAAPIDecoderContext*>(ist->hwaccel_ctx);

  int err = av_hwframe_get_buffer(ctx->frames_ref, frame, 0);
  if (err < 0) {
    ERROR_LOG() << "Failed to allocate decoder surface.";
  } else {
    DEBUG_LOG() << "Decoder given surface " << reinterpret_cast<uintptr_t>(frame->data[3]) << ".";
  }
  return err;
}

static int vaapi_retrieve_data(AVCodecContext* avctx, AVFrame* input) {
  InputStream* ist = static_cast<InputStream*>(avctx->opaque);
  VAAPIDecoderContext* ctx = static_cast<VAAPIDecoderContext*>(ist->hwaccel_ctx);

  if (ctx->output_format == AV_PIX_FMT_VAAPI) {
    return 0;
  }

  AVFrame* output = av_frame_alloc();
  if (!output) {
    return AVERROR(ENOMEM);
  }

  output->format = ctx->output_format;

  int err = av_hwframe_transfer_data(output, input, 0);
  if (err < 0) {
    ERROR_LOG() << "Failed to transfer data to output frame error: " << err << ".";
    av_frame_free(&output);
    return err;
  }

  err = av_frame_copy_props(output, input);
  if (err < 0) {
    av_frame_unref(output);
    av_frame_free(&output);
    return err;
  }

  av_frame_unref(input);
  av_frame_move_ref(input, output);
  av_frame_free(&output);
  return 0;
}

static int vaapi_device_init(const char* device) {
  av_buffer_unref(&hw_device_ctx);
  int err = av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_VAAPI, device, NULL, 0);
  if (err < 0) {
    return err;
  }

  return 0;
}

int vaapi_init(AVCodecContext* decoder_ctx) {
  InputStream* ist = static_cast<InputStream*>(decoder_ctx->opaque);
  if (ist->hwaccel_ctx) {
    vaapi_uninit(decoder_ctx);
  }

  // We have -hwaccel without -vaapi_device, so just initialise here with
  // the device passed as -hwaccel_device (if -vaapi_device was passed, it
  // will always have been called before now).
  if (!hw_device_ctx) {
    int err = vaapi_device_init(ist->hwaccel_device);
    if (err < 0) {
      ERROR_LOG() << "VAAPI init failed error: " << err;
      return err;
    }
  }

  VAAPIDecoderContext* ctx = static_cast<VAAPIDecoderContext*>(av_mallocz(sizeof(*ctx)));
  if (!ctx) {
    ERROR_LOG() << "Failed to create VAAPI context.";
    return AVERROR(ENOMEM);
  }

  ist->hwaccel_ctx = ctx;
  ctx->device_ref = av_buffer_ref(hw_device_ctx);
  ctx->device = reinterpret_cast<AVHWDeviceContext*>(ctx->device_ref->data);

  ctx->output_format = ist->hwaccel_output_format;
  decoder_ctx->pix_fmt = ctx->output_format;

  ctx->frames_ref = av_hwframe_ctx_alloc(ctx->device_ref);
  if (!ctx->frames_ref) {
    ERROR_LOG() << "Failed to create VAAPI frame context.";
    vaapi_uninit(decoder_ctx);
    return AVERROR(ENOMEM);
  }

  ctx->frames = reinterpret_cast<AVHWFramesContext*>(ctx->frames_ref->data);
  ctx->frames->format = AV_PIX_FMT_VAAPI;
  ctx->frames->width = decoder_ctx->coded_width;
  ctx->frames->height = decoder_ctx->coded_height;

  // It would be nice if we could query the available formats here,
  // but unfortunately we don't have a VAConfigID to do it with.
  // For now, just assume an NV12 format (or P010 if 10-bit).
  ctx->frames->sw_format = (decoder_ctx->sw_pix_fmt == AV_PIX_FMT_YUV420P10 ? AV_PIX_FMT_P010 : AV_PIX_FMT_NV12);

  // For frame-threaded decoding, at least one additional surface
  // is needed for each thread.
  ctx->frames->initial_pool_size = DEFAULT_SURFACES;
  if (decoder_ctx->active_thread_type & FF_THREAD_FRAME) {
    ctx->frames->initial_pool_size += decoder_ctx->thread_count;
  }

  int err = av_hwframe_ctx_init(ctx->frames_ref);
  if (err < 0) {
    ERROR_LOG() << "Failed to initialise VAAPI hw_frame context error: " << err;
    vaapi_uninit(decoder_ctx);
    return err;
  }

  ist->hw_frames_ctx = av_buffer_ref(ctx->frames_ref);
  if (!ist->hw_frames_ctx) {
    ERROR_LOG() << "Failed to create VAAPI hw_frame context.";
    vaapi_uninit(decoder_ctx);
    return AVERROR(ENOMEM);
  }

  ist->hwaccel_uninit = &vaapi_uninit;
  ist->hwaccel_get_buffer = &vaapi_get_buffer;
  ist->hwaccel_retrieve_data = &vaapi_retrieve_data;

  INFO_LOG() << "Using VAAPI to decode input stream.";
  return 0;
}

void vaapi_uninit(AVCodecContext* decoder_ctx) {
  InputStream* ist = static_cast<InputStream*>(decoder_ctx->opaque);
  VAAPIDecoderContext* ctx = static_cast<VAAPIDecoderContext*>(ist->hwaccel_ctx);

  if (ctx) {
    av_buffer_unref(&ctx->frames_ref);
    av_buffer_unref(&ctx->device_ref);
    av_free(ctx);
  }

  av_buffer_unref(&ist->hw_frames_ctx);

  ist->hwaccel_ctx = NULL;
  ist->hwaccel_uninit = NULL;
  ist->hwaccel_get_buffer = NULL;
  ist->hwaccel_retrieve_data = NULL;
}

}  // namespace media
}  // namespace player
}  // namespace client
}  // namespace fastotv
