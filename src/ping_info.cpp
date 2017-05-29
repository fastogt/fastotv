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

#include "ping_info.h"

#include <string>

#include "third-party/json-c/json-c/json.h"  // for json_object_...

#include <common/convert2string.h>

namespace fasto {
namespace fastotv {

ServerPingInfo::ServerPingInfo() : timestamp(common::time::current_utc_mstime()) {
}

struct json_object* ServerPingInfo::MakeJobject(const ServerPingInfo& inf) {
  json_object* obj = json_object_new_object();
  json_object_object_add(obj, SERVER_INFO_TIMESTAMP_FIELD, json_object_new_int64(inf.timestamp));
  return obj;
}

ServerPingInfo ServerPingInfo::MakeClass(json_object* obj) {
  json_object* jtimestamp = NULL;
  json_bool jtimestamp_exists =
      json_object_object_get_ex(obj, SERVER_INFO_TIMESTAMP_FIELD, &jtimestamp);
  ServerPingInfo inf;
  if (jtimestamp_exists) {
    inf.timestamp = json_object_get_int64(jtimestamp);
  }
  return inf;
}

ClientPingInfo::ClientPingInfo() : timestamp(common::time::current_utc_mstime()) {
}

struct json_object* ClientPingInfo::MakeJobject(const ClientPingInfo& inf) {
  json_object* obj = json_object_new_object();
  json_object_object_add(obj, CLIENT_INFO_TIMESTAMP_FIELD, json_object_new_int64(inf.timestamp));
  return obj;
}

ClientPingInfo ClientPingInfo::MakeClass(json_object* obj) {
  json_object* jtimestamp = NULL;
  json_bool jtimestamp_exists =
      json_object_object_get_ex(obj, CLIENT_INFO_TIMESTAMP_FIELD, &jtimestamp);
  ClientPingInfo inf;
  if (jtimestamp_exists) {
    inf.timestamp = json_object_get_int64(jtimestamp);
  }
  return inf;
}

}  // namespace fastotv
}  // namespace fasto
