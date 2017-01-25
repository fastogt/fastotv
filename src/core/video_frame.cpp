#include "core/video_frame.h"

namespace core {

VideoFrame::VideoFrame()
    : frame(av_frame_alloc()),
      serial(0),
      pts(0),
      duration(0),
      pos(0),
      bmp(NULL),
      allocated(0),
      width(0),
      height(0),
      format(0),
      sar{0, 0},
      uploaded(0),
      flip_v(0) {}

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

double VideoFrame::VpDuration(core::VideoFrame* vp,
                              core::VideoFrame* nextvp,
                              double max_frame_duration) {
  if (vp->serial == nextvp->serial) {
    double duration = nextvp->pts - vp->pts;
    if (isnan(duration) || duration <= 0 || duration > max_frame_duration) {
      return vp->duration;
    } else {
      return duration;
    }
  } else {
    return 0.0;
  }
}

}  // namespace core
