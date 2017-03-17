#include "client/core/events/stream_events.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
namespace events {

StreamInfo::StreamInfo(VideoState* stream) : stream_(stream) {}

FrameInfo::FrameInfo(VideoState* stream, core::VideoFrame* frame)
    : StreamInfo(stream), frame(frame) {}

QuitStreamInfo::QuitStreamInfo(VideoState* stream, int code) : StreamInfo(stream), code(code) {}
}
}
}
}
}
