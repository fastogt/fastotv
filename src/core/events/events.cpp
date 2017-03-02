#include "core/events/events.h"

#include <common/time.h>

namespace core {
namespace events {

StreamInfo::StreamInfo(VideoState* stream) : stream_(stream) {}

FrameInfo::FrameInfo(VideoState* stream, core::VideoFrame* frame)
    : StreamInfo(stream), frame(frame) {}

QuitStreamInfo::QuitStreamInfo(VideoState* stream, int code) : StreamInfo(stream), code(code) {}

ChangeStreamInfo::ChangeStreamInfo(VideoState* stream, ChangeDirection direction, size_t pos)
    : StreamInfo(stream), direction(direction), pos(pos) {}

TimeInfo::TimeInfo() : time_millisecond(common::time::current_mstime()) {}

KeyPressInfo::KeyPressInfo(bool pressed, Keysym ks) : is_pressed(pressed), ks(ks) {}

KeyReleaseInfo::KeyReleaseInfo(bool pressed, Keysym ks) : is_pressed(pressed), ks(ks) {}

WindowResizeInfo::WindowResizeInfo(int width, int height) : width(width), height(height) {}

MousePressInfo::MousePressInfo(uint8_t button, uint8_t state) : button(button), state(state) {}

MouseReleaseInfo::MouseReleaseInfo(uint8_t button, uint8_t state) : button(button), state(state) {}
}
}
