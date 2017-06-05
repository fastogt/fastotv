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

#include <common/net/types.h>  // for HostAndPort

#include "client/core/events/events_base.h"  // for EventBase, EventsType::C...
#include "client/types.h"                    // for BandwidthHostType
#include "client_server_types.h"             // for bandwidth_t

namespace fasto {
namespace fastotv {
class AuthInfo;
}
}  // namespace fasto
namespace fasto {
namespace fastotv {
class ChannelsInfo;
}
}  // namespace fasto

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
namespace events {

class TvConfig {};
struct BandwidtInfo {
  BandwidtInfo();
  explicit BandwidtInfo(const common::net::HostAndPort& host, bandwidth_t band, BandwidthHostType hs);

  common::net::HostAndPort host;
  bandwidth_t bandwidth;
  BandwidthHostType host_type;
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
typedef EventBase<CLIENT_RECEIVE_CHANNELS_EVENT, ChannelsInfo> ReceiveChannelsEvent;
typedef EventBase<CLIENT_BANDWIDTH_ESTIMATION_EVENT, BandwidtInfo> BandwidthEstimationEvent;

}  // namespace events
}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
