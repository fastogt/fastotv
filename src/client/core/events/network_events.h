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

#include <common/net/types.h>

#include "client/core/events/events_base.h"

#include "auth_info.h"
#include "channel_info.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
namespace events {

class TvConfig {};
struct BandwidtInfo {
  BandwidtInfo();
  explicit BandwidtInfo(const common::net::HostAndPort& host, bandwidth_t band, BandWidthHostType hs);

  common::net::HostAndPort host;
  bandwidth_t bandwidth;
  BandWidthHostType host_type;
};
struct ConnectInfo {
  ConnectInfo();
  explicit ConnectInfo(const common::net::HostAndPort& host);

  common::net::HostAndPort host;
};

typedef EventBase<CLIENT_DISCONNECT_EVENT, ConnectInfo> ClientDisconnectedEvent;
typedef EventBase<CLIENT_CONNECT_EVENT, ConnectInfo> ClientConnectedEvent;
typedef EventBase<CLIENT_AUTHORIZED_EVENT, AuthInfo> ClientAuthorizedEvent;
typedef EventBase<CLIENT_UNAUTHORIZED_EVENT, AuthInfo> ClientUnAuthorizedEvent;
typedef EventBase<CLIENT_CONFIG_CHANGE_EVENT, TvConfig> ClientConfigChangeEvent;
typedef EventBase<CLIENT_RECEIVE_CHANNELS_EVENT, channels_t> ReceiveChannelsEvent;
typedef EventBase<CLIENT_BANDWIDTH_ESTIMATION_EVENT, BandwidtInfo> BandwidthEstimationEvent;

}  // namespace events {
}
}
}
}
