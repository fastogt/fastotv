/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "client/core/hwaccels/ffmpeg_vdpau.h"

extern "C" {
#include <libavcodec/vdpau.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vdpau.h>
}

#include <common/macros.h>

#include "client/core/ffmpeg_internal.h"

#define DEFAULT_SURFACES 20

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

typedef struct VDPAUContext {
  AVBufferRef* hw_frames_ctx;
  enum AVPixelFormat output_format;
} VDPAUContext;

typedef void vdpau_uninit_callback_t(AVCodecContext* s);
typedef int vdpau_get_buffer_callback_t(AVCodecContext* s, AVFrame* frame, int flags);
typedef int vdpau_retrieve_data_callback_t(AVCodecContext* s, AVFrame* frame);

static void vdpau_uninit(AVCodecContext* s) {
  InputStream* ist = static_cast<InputStream*>(s->opaque);
  VDPAUContext* ctx = static_cast<VDPAUContext*>(ist->hwaccel_ctx);

  ist->hwaccel_uninit = NULL;
  ist->hwaccel_get_buffer = NULL;
  ist->hwaccel_retrieve_data = NULL;

  av_buffer_unref(&ctx->hw_frames_ctx);

  av_freep(&ist->hwaccel_ctx);
  av_freep(&s->hwaccel_context);
}

static int vdpau_get_buffer(AVCodecContext* s, AVFrame* frame, int flags) {
  UNUSED(flags);
  InputStream* ist = static_cast<InputStream*>(s->opaque);
  VDPAUContext* ctx = static_cast<VDPAUContext*>(ist->hwaccel_ctx);

  int err = av_hwframe_get_buffer(ctx->hw_frames_ctx, frame, 0);
  if (err < 0) {
    ERROR_LOG() << "Failed to allocate decoder surface.";
  } else {
    DEBUG_LOG() << "Decoder given surface " << reinterpret_cast<uintptr_t>(frame->data[3]) << ".";
  }
  return err;
}

static int vdpau_retrieve_data(AVCodecContext* avctx, AVFrame* input) {
  InputStream* ist = static_cast<InputStream*>(avctx->opaque);
  VDPAUContext* ctx = static_cast<VDPAUContext*>(ist->hwaccel_ctx);

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

static int vaapi_decode_init(AVCodecContext* avctx) {
  InputStream* ist = static_cast<InputStream*>(avctx->opaque);

  VDPAUContext* ctx = static_cast<VDPAUContext*>(av_mallocz(sizeof(*ctx)));
  if (!ctx) {
    ERROR_LOG() << "VDPAU init failed(alloc VDPAUContext).";
    vdpau_uninit(avctx);
    return AVERROR(ENOMEM);
  }

  ist->hwaccel_ctx = ctx;

  AVBufferRef* device_ref = NULL;
  int ret =
      av_hwdevice_ctx_create(&device_ref, AV_HWDEVICE_TYPE_VDPAU, ist->hwaccel_device, NULL, 0);
  if (ret < 0) {
    ERROR_LOG() << "VDPAU init failed error: " << ret;
    vdpau_uninit(avctx);
    return AVERROR(EINVAL);
  }
  AVHWDeviceContext* device_ctx = reinterpret_cast<AVHWDeviceContext*>(device_ref->data);
  AVVDPAUDeviceContext* device_hwctx = static_cast<AVVDPAUDeviceContext*>(device_ctx->hwctx);

  ctx->output_format = ist->hwaccel_output_format;
  avctx->pix_fmt = ctx->output_format;
  ctx->hw_frames_ctx = av_hwframe_ctx_alloc(device_ref);
  if (!ctx->hw_frames_ctx) {
    ERROR_LOG() << "Failed to create VDPAU frame context.";
    vdpau_uninit(avctx);
    return AVERROR(ENOMEM);
  }

  av_buffer_unref(&device_ref);

  AVHWFramesContext* frames_ctx = reinterpret_cast<AVHWFramesContext*>(ctx->hw_frames_ctx->data);
  frames_ctx->format = AV_PIX_FMT_VDPAU;
  frames_ctx->sw_format = avctx->sw_pix_fmt;
  frames_ctx->width = avctx->coded_width;
  frames_ctx->height = avctx->coded_height;

  // For frame-threaded decoding, at least one additional surface
  // is needed for each thread.
  frames_ctx->initial_pool_size = DEFAULT_SURFACES;
  if (avctx->active_thread_type & FF_THREAD_FRAME) {
    frames_ctx->initial_pool_size += avctx->thread_count;
  }

  ret = av_hwframe_ctx_init(ctx->hw_frames_ctx);
  if (ret < 0) {
    ERROR_LOG() << "Failed to initialise VDPAU hw_frame context error: " << ret;
    vdpau_uninit(avctx);
    return AVERROR(EINVAL);
  }

  ret = av_vdpau_bind_context(avctx, device_hwctx->device, device_hwctx->get_proc_address, 0);
  if (ret != 0) {
    ERROR_LOG() << "Failed to bind VDPAU context error: " << ret;
    vdpau_uninit(avctx);
    return AVERROR(EINVAL);
  }

  ist->hwaccel_uninit = &vdpau_uninit;
  ist->hwaccel_get_buffer = &vdpau_get_buffer;
  ist->hwaccel_retrieve_data = &vdpau_retrieve_data;
  INFO_LOG() << "Using VDPAU to decode input stream.";
  return 0;
}

int vdpau_init(AVCodecContext* decoder_ctx) {
  InputStream* ist = static_cast<InputStream*>(decoder_ctx->opaque);

  if (ist->hwaccel_ctx) {
    vdpau_uninit(decoder_ctx);
  }

  int ret = vaapi_decode_init(decoder_ctx);
  if (ret < 0) {
    return ret;
  }

  return 0;
}
}
}
}
}
