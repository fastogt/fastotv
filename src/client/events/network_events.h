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

#include <player/gui/events_base.h>  // for EventBase, EventsType::C...

#include "auth_info.h"
#include "channels_info.h"
#include "runtime_channel_info.h"

#include "client/types.h"         // for BandwidthHostType
#include "client_server_types.h"  // for bandwidth_t

#define CLIENT_DISCONNECT_EVENT static_cast<EventsType>(USER_EVENTS + 1)
#define CLIENT_CONNECT_EVENT static_cast<EventsType>(USER_EVENTS + 2)
#define CLIENT_AUTHORIZED_EVENT static_cast<EventsType>(USER_EVENTS + 3)
#define CLIENT_UNAUTHORIZED_EVENT static_cast<EventsType>(USER_EVENTS + 4)
#define CLIENT_CONFIG_CHANGE_EVENT static_cast<EventsType>(USER_EVENTS + 5)
#define CLIENT_RECEIVE_CHANNELS_EVENT static_cast<EventsType>(USER_EVENTS + 6)
#define CLIENT_RECEIVE_RUNTIME_CHANNELS_EVENT static_cast<EventsType>(USER_EVENTS + 7)
#define CLIENT_CHAT_MESSAGE_SENT_EVENT static_cast<EventsType>(USER_EVENTS + 8)
#define CLIENT_CHAT_MESSAGE_RECEIVE_EVENT static_cast<EventsType>(USER_EVENTS + 9)
#define CLIENT_BANDWIDTH_ESTIMATION_EVENT static_cast<EventsType>(USER_EVENTS + 10)

namespace fastotv {
namespace client {
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

typedef fastoplayer::gui::events::EventBase<CLIENT_DISCONNECT_EVENT, ConnectInfo> ClientDisconnectedEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_CONNECT_EVENT, ConnectInfo> ClientConnectedEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_AUTHORIZED_EVENT, AuthInfo> ClientAuthorizedEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_UNAUTHORIZED_EVENT, AuthInfo> ClientUnAuthorizedEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_CONFIG_CHANGE_EVENT, TvConfig> ClientConfigChangeEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_RECEIVE_CHANNELS_EVENT, ChannelsInfo> ReceiveChannelsEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_RECEIVE_RUNTIME_CHANNELS_EVENT, RuntimeChannelInfo>
    ReceiveRuntimeChannelEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_CHAT_MESSAGE_SENT_EVENT, ChatMessage> SendChatMessageEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_CHAT_MESSAGE_RECEIVE_EVENT, ChatMessage> ReceiveChatMessageEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_BANDWIDTH_ESTIMATION_EVENT, BandwidtInfo> BandwidthEstimationEvent;

}  // namespace events
}  // namespace client
}  // namespace fastotv
