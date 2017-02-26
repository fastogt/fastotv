#include "events.h"

IBaseEvent::~IBaseEvent() {}

IBaseEvent::senders_t* IBaseEvent::sender() const {
  return sender_;
}

IBaseEvent::IBaseEvent(senders_t* sender, type_t event_t)
    : IEvent<EventsType>(event_t), sender_(sender) {}

StreamEvent::StreamEvent(senders_t* sender, type_t event_t, stream_t* stream)
    : base_class(sender, event_t), stream_(stream) {}

StreamEvent::stream_t* StreamEvent::Stream() const {
  return stream_;
}

AllocFrameEvent::AllocFrameEvent(senders_t* sender, stream_t* stream, core::VideoFrame* frame)
    : base_class(sender, ALLOC_FRAME_EVENT, stream), frame_(frame) {}

core::VideoFrame* AllocFrameEvent::Frame() const {
  return frame_;
}

QuitStreamEvent::QuitStreamEvent(senders_t* sender, stream_t* stream, int code)
    : base_class(sender, QUIT_STREAM_EVENT, stream), code_(code) {}

int QuitStreamEvent::Code() const {
  return code_;
}
