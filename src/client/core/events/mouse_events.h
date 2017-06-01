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

#include "client/core/events/events_base.h"

/**
 *  Used as a mask when testing buttons in buttonstate.
 *   - Button 1:  Left mouse button
 *   - Button 2:  Middle mouse button
 *   - Button 3:  Right mouse button
 */
#define FASTO_BUTTON(X) (1 << ((X)-1))
#define FASTO_BUTTON_LEFT 1
#define FASTO_BUTTON_MIDDLE 2
#define FASTO_BUTTON_RIGHT 3
#define FASTO_BUTTON_X1 4
#define FASTO_BUTTON_X2 5
#define FASTO_BUTTON_LMASK FASTO_BUTTON(FASTO_BUTTON_LEFT)
#define FASTO_BUTTON_MMASK FASTO_BUTTON(FASTO_BUTTON_MIDDLE)
#define FASTO_BUTTON_RMASK FASTO_BUTTON(FASTO_BUTTON_RIGHT)
#define FASTO_BUTTON_X1MASK FASTO_BUTTON(FASTO_BUTTON_X1)
#define FASTO_BUTTON_X2MASK FASTO_BUTTON(FASTO_BUTTON_X2)

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
namespace events {

struct MouseMoveInfo {};

struct MousePressInfo {
  MousePressInfo(uint8_t button, uint8_t state);

  uint8_t button; /**< The mouse button index */
  uint8_t state;
};
struct MouseReleaseInfo {
  MouseReleaseInfo(uint8_t button, uint8_t state);

  uint8_t button; /**< The mouse button index */
  uint8_t state;
};

typedef EventBase<MOUSE_MOVE_EVENT, MouseMoveInfo> MouseMoveEvent;
typedef EventBase<MOUSE_PRESS_EVENT, MousePressInfo> MousePressEvent;
typedef EventBase<MOUSE_RELEASE_EVENT, MouseReleaseInfo> MouseReleaseEvent;

}  // namespace events
}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
