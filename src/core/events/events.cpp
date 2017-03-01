#include "core/events/events.h"

#include <common/time.h>

namespace core {
namespace events {

StreamInfo::StreamInfo(VideoState* stream) : stream_(stream) {}

FrameInfo::FrameInfo(VideoState* stream, core::VideoFrame* frame)
    : StreamInfo(stream), frame_(frame) {}

QuitInfo::QuitInfo(VideoState* stream, int code) : StreamInfo(stream), code_(code) {}

TimeInfo::TimeInfo() : time_millisecond(common::time::current_mstime()) {}

KeyPressInfo::KeyPressInfo(bool pressed, Keysym ks) : is_pressed(pressed), ks(ks) {}

KeyReleaseInfo::KeyReleaseInfo(bool pressed, Keysym ks) : is_pressed(pressed), ks(ks) {}
}
}
