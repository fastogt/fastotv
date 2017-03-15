#pragma once

#include "client/core/events/events_base.h"

#include "client/core/events/key_events.h"
#include "client/core/events/mouse_events.h"
#include "client/core/events/window_events.h"
#include "client/core/events/stream_events.h"
#include "client/core/events/network_events.h"

namespace fasto {
namespace fastotv {
namespace core {
namespace events {

struct TimeInfo {
  TimeInfo();

  common::time64_t time_millisecond;
};

struct QuitInfo {};

struct PreExecInfo {
  explicit PreExecInfo(int code);
  int code;
};
struct PostExecInfo {
  explicit PostExecInfo(int code);
  int code;
};

typedef EventBase<PRE_EXEC_EVENT, PreExecInfo> PreExecEvent;
typedef EventBase<POST_EXEC_EVENT, PostExecInfo> PostExecEvent;

typedef EventBase<TIMER_EVENT, TimeInfo> TimerEvent;

typedef EventBase<QUIT_EVENT, QuitInfo> QuitEvent;

}  // namespace events {
}
}
}
