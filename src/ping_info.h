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

#include "client_server_types.h"

struct json_object;

#define SERVER_INFO_TIMESTAMP_FIELD "timestamp"
#define CLIENT_INFO_TIMESTAMP_FIELD "timestamp"

namespace fasto {
namespace fastotv {

struct ServerPingInfo {
  ServerPingInfo();

  static json_object* MakeJobject(const ServerPingInfo& inf);  // allocate json_object
  static ServerPingInfo MakeClass(json_object* obj);           // pass valid json obj

  timestamp_t timestamp;  // utc time
};

struct ClientPingInfo {
  ClientPingInfo();

  static json_object* MakeJobject(const ClientPingInfo& inf);  // allocate json_object
  static ClientPingInfo MakeClass(json_object* obj);           // pass valid json obj

  timestamp_t timestamp;  // utc time
};

}  // namespace fastotv
}  // namespace fasto
