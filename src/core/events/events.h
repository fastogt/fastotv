#pragma once

#include "core/events/events_base.h"

#include "core/events/key_events.h"
#include "core/events/mouse_events.h"
#include "core/events/window_events.h"
#include "core/events/stream_events.h"

namespace core {
namespace events {

struct TimeInfo {
  TimeInfo();

  common::time64_t time_millisecond;
};

struct QuitInfo {};

typedef EventBase<TIMER_EVENT, TimeInfo> TimerEvent;

typedef EventBase<QUIT_EVENT, QuitInfo> QuitEvent;

}  // namespace events {
}
