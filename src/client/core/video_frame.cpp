#include "client/core/video_frame.h"

#include <math.h>    // for isnan
#include <stddef.h>  // for NULL

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

VideoFrame::VideoFrame()
    : frame(av_frame_alloc()),
      serial(0),
      pts(0),
      duration(0),
      pos(0),
      bmp(NULL),
      allocated(false),
      width(0),
      height(0),
      format(0),
      sar{0, 0},
      uploaded(false),
      flip_v(false) {}

VideoFrame::~VideoFrame() {
  ClearFrame();
  av_frame_free(&frame);
  if (bmp) {
    SDL_DestroyTexture(bmp);
    bmp = NULL;
  }
}

void VideoFrame::ClearFrame() {
  av_frame_unref(frame);
}

clock_t VideoFrame::VpDuration(core::VideoFrame* vp,
                               core::VideoFrame* nextvp,
                               clock_t max_frame_duration) {
  if (vp->serial == nextvp->serial) {
    clock_t duration = nextvp->pts - vp->pts;
    if (!IsValidClock(duration) || duration <= 0 || duration > max_frame_duration) {
      return vp->duration;
    } else {
      return duration;
    }
  } else {
    return 0.0;
  }
}

}  // namespace core
}
}
}
