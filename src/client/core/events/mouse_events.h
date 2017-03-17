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

}  // namespace events {
}
}
}
}
