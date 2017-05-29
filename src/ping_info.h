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
#include "serializer/json_serializer.h"

#define SERVER_INFO_TIMESTAMP_FIELD "timestamp"
#define CLIENT_INFO_TIMESTAMP_FIELD "timestamp"

namespace fasto {
namespace fastotv {

struct ServerPingInfo : public JsonSerializer<ServerPingInfo> {
  ServerPingInfo();

  common::Error Serialize(serialize_type* deserialized) const WARN_UNUSED_RESULT;
  static common::Error DeSerialize(const serialize_type& serialized,
                                   value_type* obj) WARN_UNUSED_RESULT;

  timestamp_t timestamp;  // utc time
};

struct ClientPingInfo : public JsonSerializer<ClientPingInfo> {
  ClientPingInfo();

  common::Error Serialize(serialize_type* deserialized) const WARN_UNUSED_RESULT;
  static common::Error DeSerialize(const serialize_type& serialized,
                                   value_type* obj) WARN_UNUSED_RESULT;

  timestamp_t timestamp;  // utc time
};

}  // namespace fastotv
}  // namespace fasto
