#include "client/core/events/key_events.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
namespace events {

KeyPressInfo::KeyPressInfo(bool pressed, Keysym ks) : is_pressed(pressed), ks(ks) {}

KeyReleaseInfo::KeyReleaseInfo(bool pressed, Keysym ks) : is_pressed(pressed), ks(ks) {}
}
}
}
}
}
