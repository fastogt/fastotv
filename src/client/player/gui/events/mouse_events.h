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

#include <stdint.h>  // for uint8_t

#include <SDL2/SDL_events.h>

#include "client/player/gui/events_base.h"  // for EventBase, EventsType::M...

namespace fastotv {
namespace client {
namespace player {
namespace core {
namespace events {

struct MouseMoveInfo {};

struct MousePressInfo {
  MousePressInfo(const SDL_MouseButtonEvent& event);

  SDL_Point GetMousePoint() const;

  SDL_MouseButtonEvent mevent;
};
struct MouseReleaseInfo {
  MouseReleaseInfo(const SDL_MouseButtonEvent& event);

  SDL_Point GetMousePoint() const;

  SDL_MouseButtonEvent mevent;
};

typedef EventBase<MOUSE_MOVE_EVENT, MouseMoveInfo> MouseMoveEvent;
typedef EventBase<MOUSE_PRESS_EVENT, MousePressInfo> MousePressEvent;
typedef EventBase<MOUSE_RELEASE_EVENT, MouseReleaseInfo> MouseReleaseEvent;

}  // namespace events
}  // namespace core
}  // namespace player
}  // namespace client
}  // namespace fastotv
