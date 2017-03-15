#include "client/core/audio_frame.h"

namespace fasto {
namespace fastotv {
namespace core {

AudioFrame::AudioFrame() : frame(av_frame_alloc()), serial(0), pts(0), duration(0), pos(0) {}

AudioFrame::~AudioFrame() {
  ClearFrame();
  av_frame_free(&frame);
}

void AudioFrame::ClearFrame() {
  av_frame_unref(frame);
}

}  // namespace core
}
}
