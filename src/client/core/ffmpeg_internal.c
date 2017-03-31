#include "ffmpeg_internal.h"

#ifdef HAVE_VDPAU
#include "ffmpeg_vdpau.h"
#endif

const HWAccel hwaccels[] = {
#if HAVE_VDPAU_X11
    {"vdpau", vdpau_init, HWACCEL_VDPAU, AV_PIX_FMT_VDPAU},
#endif
#if HAVE_DXVA2_LIB
    {"dxva2", dxva2_init, HWACCEL_DXVA2, AV_PIX_FMT_DXVA2_VLD},
#endif
#if CONFIG_VDA
    {"vda", videotoolbox_init, HWACCEL_VDA, AV_PIX_FMT_VDA},
#endif
#if CONFIG_VIDEOTOOLBOX
    {"videotoolbox", videotoolbox_init, HWACCEL_VIDEOTOOLBOX, AV_PIX_FMT_VIDEOTOOLBOX},
#endif
#if CONFIG_LIBMFX
    {"qsv", qsv_init, HWACCEL_QSV, AV_PIX_FMT_QSV},
#endif
#if CONFIG_VAAPI
    {"vaapi", vaapi_decode_init, HWACCEL_VAAPI, AV_PIX_FMT_VAAPI},
#endif
#if CONFIG_CUVID
    {"cuvid", cuvid_init, HWACCEL_CUVID, AV_PIX_FMT_CUDA},
#endif
    {0},
};

size_t hwaccel_count() {
  return FF_ARRAY_ELEMS(hwaccels) - 1;
}

const HWAccel* get_hwaccel(enum AVPixelFormat pix_fmt) {
  for (int i = 0; hwaccels[i].name; i++) {
    if (hwaccels[i].pix_fmt == pix_fmt) {
      return &hwaccels[i];
    }
  }
  return NULL;
}
