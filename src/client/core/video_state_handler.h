#pragma once

extern "C" {
#include <libavutil/rational.h>
}

#include "client/core/events/events_base.h"

namespace fasto {
namespace fastotv {
namespace core {

struct AudioParams;
class VideoState;
struct VideoFrame;

class VideoStateHandler : public core::events::EventListener {
 public:
  VideoStateHandler();
  virtual void HandleEvent(event_t* event) override = 0;
  virtual void HandleExceptionEvent(event_t* event, common::Error err) override = 0;

  virtual ~VideoStateHandler();

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
}
}
}
