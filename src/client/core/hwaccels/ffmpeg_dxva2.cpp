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

#include "client/core/hwaccels/ffmpeg_dxva2.h"

#include <windows.h>
#undef ERROR

#define DXVA2API_USE_BITFIELDS
#define COBJMACROS
#define CINTERFACE

#include <d3d9.h>
#include <dxva2api.h>

extern "C" {

#include <libavcodec/dxva2.h>

#include <libavutil/avassert.h>
#include <libavutil/buffer.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>

#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_dxva2.h>
}

/* define all the GUIDs used directly here,
   to avoid problems with inconsistent dxva2api.h versions in mingw-w64 and different MSVC version
   */
#include <initguid.h>

#include <common/macros.h>

#include "client/core/ffmpeg_internal.h"

DEFINE_GUID(IID_IDirectXVideoDecoderService,
            0xfc51a551,
            0xd5e7,
            0x11d9,
            0xaf,
            0x55,
            0x00,
            0x05,
            0x4e,
            0x43,
            0xff,
            0x02);

DEFINE_GUID(DXVA2_ModeMPEG2_VLD,
            0xee27417f,
            0x5e28,
            0x4e65,
            0xbe,
            0xea,
            0x1d,
            0x26,
            0xb5,
            0x08,
            0xad,
            0xc9);
DEFINE_GUID(DXVA2_ModeMPEG2and1_VLD,
            0x86695f12,
            0x340e,
            0x4f04,
            0x9f,
            0xd3,
            0x92,
            0x53,
            0xdd,
            0x32,
            0x74,
            0x60);
DEFINE_GUID(DXVA2_ModeH264_E,
            0x1b81be68,
            0xa0c7,
            0x11d3,
            0xb9,
            0x84,
            0x00,
            0xc0,
            0x4f,
            0x2e,
            0x73,
            0xc5);
DEFINE_GUID(DXVA2_ModeH264_F,
            0x1b81be69,
            0xa0c7,
            0x11d3,
            0xb9,
            0x84,
            0x00,
            0xc0,
            0x4f,
            0x2e,
            0x73,
            0xc5);
DEFINE_GUID(DXVADDI_Intel_ModeH264_E,
            0x604F8E68,
            0x4951,
            0x4C54,
            0x88,
            0xFE,
            0xAB,
            0xD2,
            0x5C,
            0x15,
            0xB3,
            0xD6);
DEFINE_GUID(DXVA2_ModeVC1_D,
            0x1b81beA3,
            0xa0c7,
            0x11d3,
            0xb9,
            0x84,
            0x00,
            0xc0,
            0x4f,
            0x2e,
            0x73,
            0xc5);
DEFINE_GUID(DXVA2_ModeVC1_D2010,
            0x1b81beA4,
            0xa0c7,
            0x11d3,
            0xb9,
            0x84,
            0x00,
            0xc0,
            0x4f,
            0x2e,
            0x73,
            0xc5);
DEFINE_GUID(DXVA2_ModeHEVC_VLD_Main,
            0x5b11d51b,
            0x2f4c,
            0x4452,
            0xbc,
            0xc3,
            0x09,
            0xf2,
            0xa1,
            0x16,
            0x0c,
            0xc0);
DEFINE_GUID(DXVA2_ModeHEVC_VLD_Main10,
            0x107af0e0,
            0xef1a,
            0x4d19,
            0xab,
            0xa8,
            0x67,
            0xa1,
            0x63,
            0x07,
            0x3d,
            0x13);
DEFINE_GUID(DXVA2_ModeVP9_VLD_Profile0,
            0x463707f8,
            0xa1d0,
            0x4585,
            0x87,
            0x6d,
            0x83,
            0xaa,
            0x6d,
            0x60,
            0xb8,
            0x9e);
DEFINE_GUID(DXVA2_NoEncrypt,
            0x1b81beD0,
            0xa0c7,
            0x11d3,
            0xb9,
            0x84,
            0x00,
            0xc0,
            0x4f,
            0x2e,
            0x73,
            0xc5);
DEFINE_GUID(GUID_NULL, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

typedef struct dxva2_mode {
  const GUID* guid;
  enum AVCodecID codec;
} dxva2_mode;

static const dxva2_mode dxva2_modes[] = {
    /* MPEG-2 */
    {&DXVA2_ModeMPEG2_VLD, AV_CODEC_ID_MPEG2VIDEO},
    {&DXVA2_ModeMPEG2and1_VLD, AV_CODEC_ID_MPEG2VIDEO},

    /* H.264 */
    {&DXVA2_ModeH264_F, AV_CODEC_ID_H264},
    {&DXVA2_ModeH264_E, AV_CODEC_ID_H264},
    /* Intel specific H.264 mode */
    {&DXVADDI_Intel_ModeH264_E, AV_CODEC_ID_H264},

    /* VC-1 / WMV3 */
    {&DXVA2_ModeVC1_D2010, AV_CODEC_ID_VC1},
    {&DXVA2_ModeVC1_D2010, AV_CODEC_ID_WMV3},
    {&DXVA2_ModeVC1_D, AV_CODEC_ID_VC1},
    {&DXVA2_ModeVC1_D, AV_CODEC_ID_WMV3},

    /* HEVC/H.265 */
    {&DXVA2_ModeHEVC_VLD_Main, AV_CODEC_ID_HEVC},
    {&DXVA2_ModeHEVC_VLD_Main10, AV_CODEC_ID_HEVC},

    /* VP8/9 */
    {&DXVA2_ModeVP9_VLD_Profile0, AV_CODEC_ID_VP9},

    {NULL, AV_CODEC_ID_NONE},
};

typedef struct DXVA2Context {
  IDirectXVideoDecoder* decoder;

  GUID decoder_guid;
  DXVA2_ConfigPictureDecode decoder_config;
  IDirectXVideoDecoderService* decoder_service;

  AVFrame* tmp_frame;

  AVBufferRef* hw_device_ctx;
  AVBufferRef* hw_frames_ctx;
} DXVA2Context;

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

static void dxva2_uninit(AVCodecContext* s) {
  InputStream* ist = static_cast<InputStream*>(s->opaque);
  DXVA2Context* ctx = static_cast<DXVA2Context*>(ist->hwaccel_ctx);

  ist->hwaccel_uninit = NULL;
  ist->hwaccel_get_buffer = NULL;
  ist->hwaccel_retrieve_data = NULL;

  if (ctx->decoder_service) {
    IDirectXVideoDecoderService_Release(ctx->decoder_service);
  }

  av_buffer_unref(&ctx->hw_frames_ctx);
  av_buffer_unref(&ctx->hw_device_ctx);

  av_frame_free(&ctx->tmp_frame);

  av_freep(&ist->hwaccel_ctx);
  av_freep(&s->hwaccel_context);
}

static int dxva2_get_buffer(AVCodecContext* s, AVFrame* frame, int flags) {
  UNUSED(flags);
  InputStream* ist = static_cast<InputStream*>(s->opaque);
  DXVA2Context* ctx = static_cast<DXVA2Context*>(ist->hwaccel_ctx);

  return av_hwframe_get_buffer(ctx->hw_frames_ctx, frame, 0);
}

static int dxva2_retrieve_data(AVCodecContext* s, AVFrame* frame) {
  InputStream* ist = static_cast<InputStream*>(s->opaque);
  DXVA2Context* ctx = static_cast<DXVA2Context*>(ist->hwaccel_ctx);

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

static int dxva2_alloc(AVCodecContext* s) {
  InputStream* ist = static_cast<InputStream*>(s->opaque);
  DXVA2Context* ctx = static_cast<DXVA2Context*>(av_mallocz(sizeof(DXVA2Context)));
  if (!ctx) {
    return AVERROR(ENOMEM);
  }

  ist->hwaccel_ctx = ctx;
  ist->hwaccel_uninit = dxva2_uninit;
  ist->hwaccel_get_buffer = dxva2_get_buffer;
  ist->hwaccel_retrieve_data = dxva2_retrieve_data;

  int ret = av_hwdevice_ctx_create(&ctx->hw_device_ctx, AV_HWDEVICE_TYPE_DXVA2, ist->hwaccel_device,
                                   NULL, 0);
  if (ret < 0) {
    dxva2_uninit(s);
    return AVERROR(EINVAL);
  }
  AVHWDeviceContext* device_ctx = reinterpret_cast<AVHWDeviceContext*>(ctx->hw_device_ctx->data);
  AVDXVA2DeviceContext* device_hwctx = static_cast<AVDXVA2DeviceContext*>(device_ctx->hwctx);

  HANDLE device_handle;
  HRESULT hr = IDirect3DDeviceManager9_OpenDeviceHandle(device_hwctx->devmgr, &device_handle);
  if (FAILED(hr)) {
    ERROR_LOG() << "DXVA2 failed to open a device handle err: " << hr;
    dxva2_uninit(s);
    return AVERROR(EINVAL);
  }

  void* pv = NULL;
  hr = IDirect3DDeviceManager9_GetVideoService(device_hwctx->devmgr, device_handle,
                                               IID_IDirectXVideoDecoderService, &pv);
  IDirect3DDeviceManager9_CloseDeviceHandle(device_hwctx->devmgr, device_handle);
  if (FAILED(hr)) {
    ERROR_LOG() << "DXVA2 failed to create IDirectXVideoDecoderService err: " << hr;
    dxva2_uninit(s);
    return AVERROR(EINVAL);
  }

  ctx->decoder_service = static_cast<IDirectXVideoDecoderService*>(pv);
  ctx->tmp_frame = av_frame_alloc();
  if (!ctx->tmp_frame) {
    dxva2_uninit(s);
    return AVERROR(ENOMEM);
  }

  s->hwaccel_context = static_cast<struct dxva_context*>(av_mallocz(sizeof(dxva_context)));
  if (!s->hwaccel_context) {
    dxva2_uninit(s);
    return AVERROR(ENOMEM);
  }

  return 0;
}

static int dxva2_get_decoder_configuration(AVCodecContext* s,
                                           const GUID& device_guid,
                                           const DXVA2_VideoDesc* desc,
                                           DXVA2_ConfigPictureDecode* config) {
  InputStream* ist = static_cast<InputStream*>(s->opaque);
  DXVA2Context* ctx = static_cast<DXVA2Context*>(ist->hwaccel_ctx);

  unsigned cfg_count = 0, best_score = 0;
  DXVA2_ConfigPictureDecode* cfg_list = NULL;
  DXVA2_ConfigPictureDecode best_cfg;
  HRESULT hr = IDirectXVideoDecoderService_GetDecoderConfigurations(
      ctx->decoder_service, device_guid, desc, NULL, &cfg_count, &cfg_list);
  if (FAILED(hr)) {
    ERROR_LOG() << "DXVA2 unable to retrieve decoder configurations err: " << hr;
    return AVERROR(EINVAL);
  }

  for (unsigned i = 0; i < cfg_count; i++) {
    DXVA2_ConfigPictureDecode* cfg = &cfg_list[i];

    unsigned score;
    if (cfg->ConfigBitstreamRaw == 1) {
      score = 1;
    } else if (s->codec_id == AV_CODEC_ID_H264 && cfg->ConfigBitstreamRaw == 2) {
      score = 2;
    } else {
      continue;
    }
    if (IsEqualGUID(cfg->guidConfigBitstreamEncryption, DXVA2_NoEncrypt)) {
      score += 16;
    }
    if (score > best_score) {
      best_score = score;
      best_cfg = *cfg;
    }
  }
  CoTaskMemFree(cfg_list);

  if (!best_score) {
    ERROR_LOG() << "DXVA2 no valid decoder configuration available err: " << hr;
    return AVERROR(EINVAL);
  }

  *config = best_cfg;
  return 0;
}

static int dxva2_create_decoder(AVCodecContext* s) {
  InputStream* ist = static_cast<InputStream*>(s->opaque);
  DXVA2Context* ctx = static_cast<DXVA2Context*>(ist->hwaccel_ctx);
  struct dxva_context* dxva_ctx = static_cast<struct dxva_context*>(s->hwaccel_context);

  const D3DFORMAT surface_format = (s->sw_pix_fmt == AV_PIX_FMT_YUV420P10)
                                       ? static_cast<D3DFORMAT>(MKTAG('P', '0', '1', '0'))
                                       : static_cast<D3DFORMAT>(MKTAG('N', 'V', '1', '2'));
  D3DFORMAT target_format = D3DFMT_UNKNOWN;
  int surface_alignment, num_surfaces;

  GUID* guid_list = NULL;
  unsigned guid_count = 0;
  HRESULT hr = IDirectXVideoDecoderService_GetDecoderDeviceGuids(ctx->decoder_service, &guid_count,
                                                                 &guid_list);
  if (FAILED(hr)) {
    ERROR_LOG() << "DXVA2 failed to retrieve decoder device GUIDs err: " << hr;
    return AVERROR(EINVAL);
  }

  GUID device_guid = GUID_NULL;
  unsigned j;
  for (unsigned i = 0; dxva2_modes[i].guid; i++) {
    D3DFORMAT* target_list = NULL;
    unsigned target_count = 0;
    const dxva2_mode* mode = &dxva2_modes[i];
    if (mode->codec != s->codec_id) {
      continue;
    }

    for (j = 0; j < guid_count; j++) {
      if (IsEqualGUID(*mode->guid, guid_list[j])) {
        break;
      }
    }
    if (j == guid_count) {
      continue;
    }

    hr = IDirectXVideoDecoderService_GetDecoderRenderTargets(ctx->decoder_service, *mode->guid,
                                                             &target_count, &target_list);
    if (FAILED(hr)) {
      continue;
    }
    for (unsigned j = 0; j < target_count; j++) {
      const D3DFORMAT format = target_list[j];
      if (format == surface_format) {
        target_format = format;
        break;
      }
    }
    CoTaskMemFree(target_list);
    if (target_format) {
      device_guid = *mode->guid;
      break;
    }
  }
  CoTaskMemFree(guid_list);

  if (IsEqualGUID(device_guid, GUID_NULL)) {
    ERROR_LOG() << "DXVA2 no decoder device for codec found";
    return AVERROR(EINVAL);
  }

  DXVA2_VideoDesc desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.SampleWidth = s->coded_width;
  desc.SampleHeight = s->coded_height;
  desc.Format = target_format;

  DXVA2_ConfigPictureDecode config;
  int ret = dxva2_get_decoder_configuration(s, device_guid, &desc, &config);
  if (ret < 0) {
    return AVERROR(EINVAL);
  }

  /* decoding MPEG-2 requires additional alignment on some Intel GPUs,
     but it causes issues for H.264 on certain AMD GPUs..... */
  if (s->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
    surface_alignment = 32;
    /* the HEVC DXVA2 spec asks for 128 pixel aligned surfaces to ensure
       all coding features have enough room to work with */
  } else if (s->codec_id == AV_CODEC_ID_HEVC) {
    surface_alignment = 128;
  } else {
    surface_alignment = 16;
  }

  /* 4 base work surfaces */
  num_surfaces = 4;

  /* add surfaces based on number of possible refs */
  if (s->codec_id == AV_CODEC_ID_H264 || s->codec_id == AV_CODEC_ID_HEVC) {
    num_surfaces += 16;
  } else if (s->codec_id == AV_CODEC_ID_VP9) {
    num_surfaces += 8;
  } else {
    num_surfaces += 2;
  }

  /* add extra surfaces for frame threading */
  if (s->active_thread_type & FF_THREAD_FRAME) {
    num_surfaces += s->thread_count;
  }

  ctx->hw_frames_ctx = av_hwframe_ctx_alloc(ctx->hw_device_ctx);
  if (!ctx->hw_frames_ctx) {
    return AVERROR(ENOMEM);
  }
  AVHWFramesContext* frames_ctx = reinterpret_cast<AVHWFramesContext*>(ctx->hw_frames_ctx->data);
  AVDXVA2FramesContext* frames_hwctx = static_cast<AVDXVA2FramesContext*>(frames_ctx->hwctx);

  frames_ctx->format = AV_PIX_FMT_DXVA2_VLD;
  frames_ctx->sw_format =
      (target_format == static_cast<D3DFORMAT>(MKTAG('P', '0', '1', '0')) ? AV_PIX_FMT_P010
                                                                          : AV_PIX_FMT_NV12);
  frames_ctx->width = FFALIGN(s->coded_width, surface_alignment);
  frames_ctx->height = FFALIGN(s->coded_height, surface_alignment);
  frames_ctx->initial_pool_size = num_surfaces;

  frames_hwctx->surface_type = DXVA2_VideoDecoderRenderTarget;

  ret = av_hwframe_ctx_init(ctx->hw_frames_ctx);
  if (ret < 0) {
    ERROR_LOG() << "DXVA2 failed to initialize the HW frames context err: " << ret;
    av_buffer_unref(&ctx->hw_frames_ctx);
    return AVERROR(EINVAL);
  }

  hr = IDirectXVideoDecoderService_CreateVideoDecoder(
      ctx->decoder_service, device_guid, &desc, &config, frames_hwctx->surfaces,
      frames_hwctx->nb_surfaces, &frames_hwctx->decoder_to_release);
  if (FAILED(hr)) {
    ERROR_LOG() << "Failed to create DXVA2 video decoder err: " << hr;
    av_buffer_unref(&ctx->hw_frames_ctx);
    return AVERROR(EINVAL);
  }

  ctx->decoder_guid = device_guid;
  ctx->decoder_config = config;

  dxva_ctx->cfg = &ctx->decoder_config;
  dxva_ctx->decoder = frames_hwctx->decoder_to_release;
  dxva_ctx->surface = frames_hwctx->surfaces;
  dxva_ctx->surface_count = frames_hwctx->nb_surfaces;

  if (IsEqualGUID(ctx->decoder_guid, DXVADDI_Intel_ModeH264_E)) {
    dxva_ctx->workaround |= FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO;
  }

  return 0;
}

int dxva2_init(AVCodecContext* s) {
  InputStream* ist = static_cast<InputStream*>(s->opaque);
  int ret;

  if (!ist->hwaccel_ctx) {
    ret = dxva2_alloc(s);
    if (ret < 0) {
      return ret;
    }
  }
  DXVA2Context* ctx = static_cast<DXVA2Context*>(ist->hwaccel_ctx);

  if (s->codec_id == AV_CODEC_ID_H264 &&
      (s->profile & ~FF_PROFILE_H264_CONSTRAINED) > FF_PROFILE_H264_HIGH) {
    ERROR_LOG() << "Unsupported H.264 profile for DXVA2 HWAccel: " << s->profile;
    return AVERROR(EINVAL);
  }

  if (s->codec_id == AV_CODEC_ID_HEVC && s->profile != FF_PROFILE_HEVC_MAIN &&
      s->profile != FF_PROFILE_HEVC_MAIN_10) {
    ERROR_LOG() << "Unsupported HEVC profile for DXVA2 HWAccel: " << s->profile;
    return AVERROR(EINVAL);
  }

  av_buffer_unref(&ctx->hw_frames_ctx);

  ret = dxva2_create_decoder(s);
  if (ret < 0) {
    ERROR_LOG() << "Error creating the DXVA2 decoder err: " << ret;
    return ret;
  }

  return 0;
}
}
}
}
}
