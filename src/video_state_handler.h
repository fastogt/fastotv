#pragma once

#include "ffmpeg_config.h"

extern "C" {
#include <libavformat/avformat.h>  // for av_find_input_format, etc
}

namespace core {
struct VideoFrame;
struct AudioParams;
}

class VideoState;

class VideoStateHandler {
 public:
  VideoStateHandler();
  virtual ~VideoStateHandler();

  virtual bool HandleRequestAudio(VideoState* stream,
                                  int64_t wanted_channel_layout,
                                  int wanted_nb_channels,
                                  int wanted_sample_rate,
                                  core::AudioParams* audio_hw_params) = 0;
  virtual bool HandleRequestWindow(VideoState* stream) = 0;
  virtual bool HandleRealocFrame(VideoState* stream, core::VideoFrame* frame) = 0;
  virtual void HanleDisplayFrame(VideoState* stream, const core::VideoFrame* frame) = 0;
  virtual void HandleDefaultWindowSize(int width, int height, AVRational sar) = 0;
};
