/*  Copyright (C) 2014-2020 FastoGT. All right reserved.

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

#include <fastotv/commands_info/auth_info.h>
#include <fastotv/commands_info/channels_info.h>
#include <fastotv/commands_info/notification_text_info.h>
#include <fastotv/commands_info/runtime_channel_info.h>
#include <fastotv/commands_info/server_info.h>
#include <fastotv/commands_info/vods_info.h>

#define CLIENT_DISCONNECT_EVENT static_cast<EventsType>(USER_EVENTS + 1)
#define CLIENT_CONNECT_EVENT static_cast<EventsType>(USER_EVENTS + 2)
#define CLIENT_AUTHORIZED_EVENT static_cast<EventsType>(USER_EVENTS + 3)
#define CLIENT_UNAUTHORIZED_EVENT static_cast<EventsType>(USER_EVENTS + 4)
#define CLIENT_SERVER_INFO_EVENT static_cast<EventsType>(USER_EVENTS + 5)
#define CLIENT_CONFIG_CHANGE_EVENT static_cast<EventsType>(USER_EVENTS + 6)
#define CLIENT_RECEIVE_CHANNELS_EVENT static_cast<EventsType>(USER_EVENTS + 7)
#define CLIENT_RECEIVE_RUNTIME_CHANNELS_EVENT static_cast<EventsType>(USER_EVENTS + 8)
#define CLIENT_CHAT_MESSAGE_SENT_EVENT static_cast<EventsType>(USER_EVENTS + 9)
#define CLIENT_CHAT_MESSAGE_RECEIVE_EVENT static_cast<EventsType>(USER_EVENTS + 10)
#define CLIENT_NOTIFICATION_TEXT_EVENT static_cast<EventsType>(USER_EVENTS + 11)

namespace fastotv {
namespace client {
namespace events {

class TvConfig {};

struct ConnectInfo {
  ConnectInfo();
  explicit ConnectInfo(const common::net::HostAndPort& host);

  common::net::HostAndPort host;
};

struct ChannelsMixInfo {
  commands_info::ChannelsInfo channels;
  commands_info::VodsInfo vods;
  commands_info::ChannelsInfo private_channels;
};

typedef fastoplayer::gui::events::EventBase<CLIENT_DISCONNECT_EVENT, ConnectInfo> ClientDisconnectedEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_CONNECT_EVENT, ConnectInfo> ClientConnectedEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_AUTHORIZED_EVENT, commands_info::AuthInfo> ClientAuthorizedEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_UNAUTHORIZED_EVENT, commands_info::AuthInfo> ClientUnAuthorizedEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_SERVER_INFO_EVENT, commands_info::ServerInfo> ClientServerInfoEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_CONFIG_CHANGE_EVENT, TvConfig> ClientConfigChangeEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_RECEIVE_CHANNELS_EVENT, ChannelsMixInfo> ReceiveChannelsEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_RECEIVE_RUNTIME_CHANNELS_EVENT, commands_info::RuntimeChannelInfo>
    ReceiveRuntimeChannelEvent;
typedef fastoplayer::gui::events::EventBase<CLIENT_NOTIFICATION_TEXT_EVENT, commands_info::NotificationTextInfo>
    NotificationTextEvent;

}  // namespace events
}  // namespace client
}  // namespace fastotv
