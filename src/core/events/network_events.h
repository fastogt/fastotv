#pragma once

#include "core/events/events_base.h"

#include "infos.h"
#include "tv_config.h"

namespace fasto {
namespace fastotv {
namespace core {
namespace events {

typedef EventBase<CLIENT_DISCONNECT_EVENT, fasto::fastotv::AuthInfo> ClientDisconnectedEvent;
typedef EventBase<CLIENT_CONNECT_EVENT, fasto::fastotv::AuthInfo> ClientConnectedEvent;
typedef EventBase<CLIENT_CONFIG_CHANGE_EVENT, fasto::fastotv::TvConfig> ClientConfigChangeEvent;

}  // namespace events {
}
}
}
