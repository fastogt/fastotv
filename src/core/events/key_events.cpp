#include "core/events/key_events.h"

namespace core {
namespace events {

KeyPressInfo::KeyPressInfo(bool pressed, Keysym ks) : is_pressed(pressed), ks(ks) {}

KeyReleaseInfo::KeyReleaseInfo(bool pressed, Keysym ks) : is_pressed(pressed), ks(ks) {}
}
}
