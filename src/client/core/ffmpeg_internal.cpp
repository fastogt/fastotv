#include "client/core/ffmpeg_internal.h"

#include <stddef.h>  // for NULL

#ifdef HAVE_VDPAU
#include "client/core/hwaccels/ffmpeg_vdpau.h"
#endif
#if HAVE_DXVA2_LIB
#include "client/core/hwaccels/ffmpeg_dxva2.h"
#endif
#if CONFIG_VAAPI
#include "client/core/hwaccels/ffmpeg_vaapi.h"
#endif
#if CONFIG_CUVID
#include "client/core/hwaccels/ffmpeg_cuvid.h"
#endif
#if CONFIG_VDA
#include "client/core/hwaccels/ffmpeg_videotoolbox.h"
#endif

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

AVBufferRef* hw_device_ctx = NULL;

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
#if HAVE_VAAPI_X11
    {"vaapi", vaapi_decode_init, HWACCEL_VAAPI, AV_PIX_FMT_VAAPI},
#endif
#if CONFIG_CUVID
    {"cuvid", cuvid_init, HWACCEL_CUVID, AV_PIX_FMT_CUDA},
#endif
    HWAccel()};

size_t hwaccel_count() {
  return FF_ARRAY_ELEMS(hwaccels) - 1;
}

const HWAccel* get_hwaccel(enum AVPixelFormat pix_fmt) {
  for (size_t i = 0; i < hwaccel_count(); i++) {
    if (hwaccels[i].pix_fmt == pix_fmt) {
      return &hwaccels[i];
    }
  }
  return NULL;
}
}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
