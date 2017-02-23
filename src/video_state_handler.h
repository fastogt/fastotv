#pragma once

#include "ffmpeg_config.h"

extern "C" {
#include <libavformat/avformat.h>  // for av_find_input_format, etc
}

namespace core {
struct VideoFrame;
}

class VideoState;

class VideoStateHandler {
 public:
  VideoStateHandler();
  virtual ~VideoStateHandler();

  virtual bool HandleRequestWindow(VideoState* stream) = 0;
  virtual bool HandleRealocFrame(VideoState* stream, core::VideoFrame* frame) = 0;
  virtual void HanleDisplayFrame(VideoState* stream, const core::VideoFrame* frame) = 0;
  virtual void HandleDefaultWindowSize(int width, int height, AVRational sar) = 0;
};
