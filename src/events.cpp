#include "events.h"

StreamInfo::StreamInfo(VideoState* stream) : stream_(stream) {}

FrameInfo::FrameInfo(VideoState* stream, core::VideoFrame* frame)
    : StreamInfo(stream), frame_(frame) {}

QuitInfo::QuitInfo(VideoState* stream, int code) : StreamInfo(stream), code_(code) {}
