#pragma once

#include "core/events/events_base.h"

namespace fasto {
namespace fastotv {
namespace core {
namespace events {

struct WindowResizeInfo {
  WindowResizeInfo(int width, int height);

  int width;
  int height;
};

struct WindowExposeInfo {};
struct WindowCloseInfo {};

typedef EventBase<WINDOW_RESIZE_EVENT, WindowResizeInfo> WindowResizeEvent;
typedef EventBase<WINDOW_EXPOSE_EVENT, WindowExposeInfo> WindowExposeEvent;
typedef EventBase<WINDOW_CLOSE_EVENT, WindowCloseInfo> WindowCloseEvent;

}  // namespace events {
}
}
}
