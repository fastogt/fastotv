#pragma once

#include "client/core/events/events_base.h"

namespace fasto {
namespace fastotv {
namespace core {
struct VideoFrame;
class VideoState;
namespace events {

struct StreamInfo {
  explicit StreamInfo(VideoState* stream);

  VideoState* stream_;
};

struct FrameInfo : public StreamInfo {
  FrameInfo(VideoState* stream, core::VideoFrame* frame);
  core::VideoFrame* frame;
};

struct QuitStreamInfo : public StreamInfo {
  QuitStreamInfo(VideoState* stream, int code);
  int code;
};

typedef EventBase<ALLOC_FRAME_EVENT, FrameInfo> AllocFrameEvent;
typedef EventBase<QUIT_STREAM_EVENT, QuitStreamInfo> QuitStreamEvent;

}  // namespace events {
}
}
}
