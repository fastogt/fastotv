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

#include "client/player/media/hwaccels/ffmpeg_vdpau.h"

#include <errno.h>   // for EINVAL, ENOMEM
#include <stddef.h>  // for NULL

extern "C" {
#include <libavcodec/vdpau.h>           // for av_vdpau_bind_context
#include <libavutil/buffer.h>           // for AVBufferRef, av_buffer_unref
#include <libavutil/error.h>            // for AVERROR
#include <libavutil/frame.h>            // for AVFrame, av_frame_unref
#include <libavutil/hwcontext.h>        // for AVHWFramesContext, AVHWDevi...
#include <libavutil/hwcontext_vdpau.h>  // for AVVDPAUDeviceContext
#include <libavutil/mem.h>              // for av_free, av_freep, av_mallocz
#include <libavutil/pixfmt.h>           // for AVPixelFormat::AV_PIX_FMT_V...
}

#include <common/logger.h>  // for COMPACT_LOG_ERROR, ERROR_LOG
#include <common/macros.h>  // for UNUSED

#include "client/player/media/ffmpeg_internal.h"  // for InputStream

namespace fastotv {
namespace client {
namespace player {
namespace media {

typedef struct VDPAUContext {
  AVBufferRef* hw_frames_ctx;
  AVFrame* tmp_frame;
} VDPAUContext;

typedef void vdpau_uninit_callback_t(AVCodecContext* s);
typedef int vdpau_get_buffer_callback_t(AVCodecContext* s, AVFrame* frame, int flags);
typedef int vdpau_retrieve_data_callback_t(AVCodecContext* s, AVFrame* frame);

static int vdpau_get_buffer(AVCodecContext* s, AVFrame* frame, int flags) {
  UNUSED(flags);
  InputStream* ist = static_cast<InputStream*>(s->opaque);
  VDPAUContext* ctx = static_cast<VDPAUContext*>(ist->hwaccel_ctx);

  return av_hwframe_get_buffer(ctx->hw_frames_ctx, frame, 0);
}

static int vdpau_retrieve_data(AVCodecContext* s, AVFrame* frame) {
  InputStream* ist = static_cast<InputStream*>(s->opaque);
  VDPAUContext* ctx = static_cast<VDPAUContext*>(ist->hwaccel_ctx);

  int ret = av_hwframe_transfer_data(ctx->tmp_frame, frame, 0);
  if (ret < 0) {
    return ret;
  }

  ret = av_frame_copy_props(ctx->tmp_frame, frame);
  if (ret < 0) {
    av_frame_unref(ctx->tmp_frame);
    return ret;
  }

  av_frame_unref(frame);
  av_frame_move_ref(frame, ctx->tmp_frame);
  return 0;
}

static int vdpau_alloc(AVCodecContext* s) {
  InputStream* ist = static_cast<InputStream*>(s->opaque);

  VDPAUContext* ctx = static_cast<VDPAUContext*>(av_mallocz(sizeof(*ctx)));
  if (!ctx) {
    ERROR_LOG() << "VDPAU init failed(alloc VDPAUContext).";
    vdpau_uninit(s);
    return AVERROR(ENOMEM);
  }

  ist->hwaccel_ctx = ctx;

  ctx->tmp_frame = av_frame_alloc();
  if (!ctx->tmp_frame) {
    ERROR_LOG() << "Failed to create VDPAU frame context.";
    vdpau_uninit(s);
    return AVERROR(ENOMEM);
  }

  AVBufferRef* device_ref = NULL;
  int ret = av_hwdevice_ctx_create(&device_ref, AV_HWDEVICE_TYPE_VDPAU, ist->hwaccel_device, NULL, 0);
  if (ret < 0) {
    ERROR_LOG() << "VDPAU init failed error: " << ret;
    vdpau_uninit(s);
    return AVERROR(EINVAL);
  }
  AVHWDeviceContext* device_ctx = reinterpret_cast<AVHWDeviceContext*>(device_ref->data);
  AVVDPAUDeviceContext* device_hwctx = static_cast<AVVDPAUDeviceContext*>(device_ctx->hwctx);

  ctx->hw_frames_ctx = av_hwframe_ctx_alloc(device_ref);
  if (!ctx->hw_frames_ctx) {
    ERROR_LOG() << "Failed to create VDPAU frame context.";
    vdpau_uninit(s);
    return AVERROR(ENOMEM);
  }

  av_buffer_unref(&device_ref);

  AVHWFramesContext* frames_ctx = reinterpret_cast<AVHWFramesContext*>(ctx->hw_frames_ctx->data);
  frames_ctx->format = AV_PIX_FMT_VDPAU;
  frames_ctx->sw_format = s->sw_pix_fmt;
  frames_ctx->width = s->coded_width;
  frames_ctx->height = s->coded_height;

  ret = av_hwframe_ctx_init(ctx->hw_frames_ctx);
  if (ret < 0) {
    ERROR_LOG() << "Failed to initialise VDPAU hw_frame context error: " << ret;
    vdpau_uninit(s);
    return AVERROR(EINVAL);
  }

  ret = av_vdpau_bind_context(s, device_hwctx->device, device_hwctx->get_proc_address, 0);
  if (ret != 0) {
    ERROR_LOG() << "Failed to bind VDPAU context error: " << ret;
    vdpau_uninit(s);
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
    return 0;
  }

  return vdpau_alloc(decoder_ctx);
}

void vdpau_uninit(AVCodecContext* decoder_ctx) {
  InputStream* ist = static_cast<InputStream*>(decoder_ctx->opaque);
  VDPAUContext* ctx = static_cast<VDPAUContext*>(ist->hwaccel_ctx);

  if (ctx) {
    av_buffer_unref(&ctx->hw_frames_ctx);
    av_frame_free(&ctx->tmp_frame);
    av_free(ctx);
  }

  ist->hwaccel_ctx = NULL;
  ist->hwaccel_uninit = NULL;
  ist->hwaccel_get_buffer = NULL;
  ist->hwaccel_retrieve_data = NULL;

  av_freep(&decoder_ctx->hwaccel_context);
}

}  // namespace media
}  // namespace player
}  // namespace client
}  // namespace fastotv
