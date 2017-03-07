#pragma once

#include <common/event.h>

enum EventsType {
  PRE_EXEC_EVENT = 0,
  POST_EXEC_EVENT,

  TIMER_EVENT,

  ALLOC_FRAME_EVENT,
  QUIT_STREAM_EVENT,

  KEY_PRESS_EVENT,
  KEY_RELEASE_EVENT,
  WINDOW_RESIZE_EVENT,
  WINDOW_EXPOSE_EVENT,
  WINDOW_CLOSE_EVENT,
  MOUSE_MOVE_EVENT,
  MOUSE_PRESS_EVENT,
  MOUSE_RELEASE_EVENT,
  QUIT_EVENT,
  COUNT_EVENTS
};
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

namespace core {
namespace events {

typedef common::event_traits<EventsType> EventTraits;
typedef EventTraits::event_t Event;
typedef EventTraits::listener_t EventListener;

// event base templates
template <EventsType event_t, typename inf_t>
class EventBase : public common::Event<EventsType, event_t> {
 public:
  typedef inf_t info_t;
  typedef common::Event<EventsType, event_t> base_class_t;
  typedef typename base_class_t::senders_t senders_t;

  EventBase(senders_t* sender, info_t info) : base_class_t(sender), info_(info) {}

  info_t info() const { return info_; }

 private:
  const info_t info_;
};

template <EventsType event_t>
class EventBase<event_t, void> : public common::Event<EventsType, event_t> {
 public:
  typedef void info_t;
  typedef common::Event<EventsType, event_t> base_class_t;
  typedef typename base_class_t::senders_t senders_t;

  explicit EventBase(senders_t* sender) : base_class_t(sender) {}
};
}
}
