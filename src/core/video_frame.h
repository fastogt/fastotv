#pragma once

#include <stdint.h>

#include <SDL2/SDL_render.h>

#include "ffmpeg_config.h"

extern "C" {
#include <libavutil/frame.h>     // for AVFrame
#include <libavutil/rational.h>  // for AVRational
}

#include <common/macros.h>  // for DISALLOW_COPY_AND_ASSIGN

#include "core/clock.h"

namespace fasto {
namespace fastotv {
namespace core {

/* Common struct for handling all types of decoded data and allocated render buffers. */
struct VideoFrame {
  VideoFrame();
  ~VideoFrame();
  void ClearFrame();

  AVFrame* frame;
  serial_id_t serial;
  clock_t pts;      /* presentation timestamp for the frame */
  clock_t duration; /* estimated duration of the frame */
  int64_t pos;      /* byte position of the frame in the input file */
  SDL_Texture* bmp;
  bool allocated;
  int width;
  int height;
  int format;
  AVRational sar;
  bool uploaded;
  bool flip_v;

  static clock_t VpDuration(core::VideoFrame* vp,
                            core::VideoFrame* nextvp,
                            clock_t max_frame_duration);

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoFrame);
};

}  // namespace core
}
}
