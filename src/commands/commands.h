/*  Copyright (C) 2014-2019 FastoGT. All right reserved.

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

#include <common/protocols/json_rpc/json_rpc.h>

// client commands
#define CLIENT_ACTIVATE "client_active"
#define CLIENT_PING "client_ping"  // ping server
#define CLIENT_GET_SERVER_INFO "get_server_info"
#define CLIENT_GET_CHANNELS "get_channels"
#define CLIENT_GET_RUNTIME_CHANNEL_INFO "get_runtime_channel_info"

// server commands
#define SERVER_PING "server_ping"  // ping client
#define SERVER_GET_CLIENT_INFO "get_client_info"

// request
// {"jsonrpc": "2.0", "method": "activate_request", "id": 11, "params": {"license_key":"%s"}}

// responce
// {"jsonrpc": "2.0", "result": 11, "id": 11}

namespace fastotv {}  // namespace fastotv
