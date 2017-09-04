/*  Copyright (C) 2014-2017 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <common/event.h>

enum EventsType : common::IEvent::event_id_t {
  PRE_EXEC_EVENT = 0,
  POST_EXEC_EVENT,

  TIMER_EVENT,

  KEY_PRESS_EVENT,
  KEY_RELEASE_EVENT,
  TEXT_INPUT_EVENT,
  TEXT_EDIT_EVENT,
  WINDOW_RESIZE_EVENT,
  WINDOW_EXPOSE_EVENT,
  WINDOW_CLOSE_EVENT,
  MOUSE_CHANGE_STATE_EVENT,
  MOUSE_MOVE_EVENT,
  MOUSE_PRESS_EVENT,
  MOUSE_RELEASE_EVENT,
  QUIT_EVENT,

  LIRC_PRESS_EVENT,

  REQUEST_VIDEO_EVENT,
  QUIT_STREAM_EVENT,

  CLIENT_CONNECT_EVENT,
  CLIENT_DISCONNECT_EVENT,
  CLIENT_AUTHORIZED_EVENT,
  CLIENT_UNAUTHORIZED_EVENT,
  CLIENT_CONFIG_CHANGE_EVENT,
  CLIENT_BANDWIDTH_ESTIMATION_EVENT,
  CLIENT_RECEIVE_CHANNELS_EVENT,
  CLIENT_RECEIVE_RUNTIME_CHANNELS_EVENT,
  CLIENT_CHAT_MESSAGE_SENT_EVENT,
  CLIENT_CHAT_MESSAGE_RECEIVE_EVENT,
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

}  // namespace common

namespace fastotv {
namespace client {
namespace player {
namespace gui {
namespace events {

typedef common::event_traits<EventsType> EventTraits;
typedef EventTraits::event_t Event;
typedef EventTraits::ex_event_t EventEx;
typedef EventTraits::listener_t EventListener;

// event base templates
template <EventsType event_t, typename inf_t>
class EventBase : public common::Event<EventsType, event_t> {
 public:
  typedef inf_t info_t;
  typedef common::Event<EventsType, event_t> base_class_t;
  typedef typename base_class_t::senders_t senders_t;

  EventBase(senders_t* sender, info_t info) : base_class_t(sender), info_(info) {}

  info_t GetInfo() const { return info_; }

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

}  // namespace events
}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
