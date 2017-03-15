#pragma once

#include "client/core/events/events_base.h"

#include "infos.h"
#include "client/tv_config.h"

namespace fasto {
namespace fastotv {
namespace core {
namespace events {

typedef EventBase<CLIENT_DISCONNECT_EVENT, fasto::fastotv::AuthInfo> ClientDisconnectedEvent;
typedef EventBase<CLIENT_CONNECT_EVENT, fasto::fastotv::AuthInfo> ClientConnectedEvent;
typedef EventBase<CLIENT_CONFIG_CHANGE_EVENT, fasto::fastotv::TvConfig> ClientConfigChangeEvent;
typedef EventBase<CLIENT_RECEIVE_CHANNELS_EVENT, channels_t> ReceiveChannelsEvent;

}  // namespace events {
}
}
}
