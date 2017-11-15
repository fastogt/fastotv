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

#include <common/protocols/three_way_handshake/commands.h>

// client commands
#define CLIENT_PING "client_ping"  // ping server
#define CLIENT_GET_SERVER_INFO "get_server_info"
#define CLIENT_GET_CHANNELS "get_channels"
#define CLIENT_GET_RUNTIME_CHANNEL_INFO "get_runtime_channel_info"
#define CLIENT_SEND_CHAT_MESSAGE "client_send_chat_message"

// server commands
#define SERVER_PING "server_ping"  // ping client
#define SERVER_WHO_ARE_YOU "who_are_you"
#define SERVER_GET_CLIENT_INFO "get_client_info"
#define SERVER_SEND_CHAT_MESSAGE "server_send_chat_message"

// request
// [uint8_t](0) [hex_string]seq [std::string]command

// responce
// [uint8_t](1) [hex_string]seq [OK|FAIL] [std::string]command args ...

// approve
// [uint8_t](2) [hex_string]seq [OK|FAIL] [std::string]command args ...

namespace fastotv {}  // namespace fastotv
