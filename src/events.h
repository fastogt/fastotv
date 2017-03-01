#pragma once

#include <common/event.h>

namespace core {
struct VideoFrame;
}

class VideoState;

enum EventsType { TIMER_EVENT, ALLOC_FRAME_EVENT, QUIT_STREAM_EVENT, COUNT_EVENTS };
#define EVENT_LOOP_ID 1

namespace common {

template <>
struct event_traits<EventsType> {
  typedef IEventEx<EventsType> event_t;
  typedef IExceptionEvent<EventsType> ex_event_t;
  typedef IListenerEx<EventsType> listener_t;
  static const unsigned max_count = COUNT_EVENTS;
  static const unsigned id = EVENT_LOOP_ID;
};
}

typedef common::event_traits<EventsType> EventTraits;
typedef EventTraits::event_t Event;
typedef EventTraits::listener_t EventListener;

template <EventsType event_t, typename inf_t>
class EventBaseInfo : public common::Event<EventsType, event_t> {
 public:
  typedef inf_t info_t;
  typedef common::Event<EventsType, event_t> base_class_t;
  typedef typename base_class_t::senders_t senders_t;

  EventBaseInfo(senders_t* sender, info_t info) : base_class_t(sender), info_(info) {}

  info_t info() const { return info_; }

 private:
  const info_t info_;
};

template <EventsType event_t>
class EventBaseInfo<event_t, void> : public common::Event<EventsType, event_t> {
 public:
  typedef void info_t;
  typedef common::Event<EventsType, event_t> base_class_t;
  typedef typename base_class_t::senders_t senders_t;

  explicit EventBaseInfo(senders_t* sender) : base_class_t(sender) {}
};

struct StreamInfo {
  explicit StreamInfo(VideoState* stream);

  VideoState* stream_;
};

struct FrameInfo : public StreamInfo {
  FrameInfo(VideoState* stream, core::VideoFrame* frame);
  core::VideoFrame* frame_;
};

struct QuitInfo : public StreamInfo {
  QuitInfo(VideoState* stream, int code);
  int code_;
};

struct TimeInfo {};

typedef EventBaseInfo<TIMER_EVENT, TimeInfo> TimerEvent;
typedef EventBaseInfo<ALLOC_FRAME_EVENT, FrameInfo> AllocFrameEvent;
typedef EventBaseInfo<QUIT_STREAM_EVENT, QuitInfo> QuitStreamEvent;
