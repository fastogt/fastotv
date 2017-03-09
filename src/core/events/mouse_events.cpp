#include "core/events/mouse_events.h"

namespace fasto {
namespace fastotv {
namespace core {
namespace events {

MousePressInfo::MousePressInfo(uint8_t button, uint8_t state) : button(button), state(state) {}

MouseReleaseInfo::MouseReleaseInfo(uint8_t button, uint8_t state) : button(button), state(state) {}
}
}
}
}
