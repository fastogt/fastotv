#pragma once

extern "C" {
#include <libavutil/rational.h>
}

#include "events.h"

namespace core {
struct AudioParams;
}

class VideoStateHandler : public EventListener {
 public:
  VideoStateHandler();
  virtual ~VideoStateHandler();

  virtual void HandleEvent(Event* event) override = 0;
  virtual void HandleExceptionEvent(Event* event, common::Error err) override = 0;

  // audio
  virtual bool HandleRequestAudio(VideoState* stream,
                                  int64_t wanted_channel_layout,
                                  int wanted_nb_channels,
                                  int wanted_sample_rate,
                                  core::AudioParams* audio_hw_params) = 0;
  virtual void HanleAudioMix(uint8_t* audio_stream_ptr,
                             const uint8_t* src,
                             uint32_t len,
                             int volume) = 0;

  // video
  virtual bool HandleRequestWindow(VideoState* stream) = 0;
  virtual bool HandleRealocFrame(VideoState* stream, core::VideoFrame* frame) = 0;
  virtual void HanleDisplayFrame(VideoState* stream, const core::VideoFrame* frame) = 0;
  virtual void HandleDefaultWindowSize(int width, int height, AVRational sar) = 0;
};
