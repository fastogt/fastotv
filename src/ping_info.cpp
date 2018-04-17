/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

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

#include <common/time.h>

#define SERVER_INFO_TIMESTAMP_FIELD "timestamp"
#define CLIENT_INFO_TIMESTAMP_FIELD "timestamp"

namespace fastotv {

ServerPingInfo::ServerPingInfo() : timestamp_(common::time::current_utc_mstime()) {}

common::Error ServerPingInfo::SerializeFields(json_object* deserialized) const {
  json_object_object_add(deserialized, SERVER_INFO_TIMESTAMP_FIELD, json_object_new_int64(timestamp_));
  return common::Error();
}

common::Error ServerPingInfo::DoDeSerialize(json_object* serialized) {
  json_object* jtimestamp = NULL;
  json_bool jtimestamp_exists = json_object_object_get_ex(serialized, SERVER_INFO_TIMESTAMP_FIELD, &jtimestamp);
  ServerPingInfo inf;
  if (jtimestamp_exists) {
    inf.timestamp_ = json_object_get_int64(jtimestamp);
  }

  *this = inf;
  return common::Error();
}

timestamp_t ServerPingInfo::GetTimeStamp() const {
  return timestamp_;
}

ClientPingInfo::ClientPingInfo() : timestamp_(common::time::current_utc_mstime()) {}

common::Error ClientPingInfo::SerializeFields(json_object* deserialized) const {
  json_object_object_add(deserialized, CLIENT_INFO_TIMESTAMP_FIELD, json_object_new_int64(timestamp_));
  return common::Error();
}

common::Error ClientPingInfo::DoDeSerialize(json_object* serialized) {
  json_object* jtimestamp = NULL;
  json_bool jtimestamp_exists = json_object_object_get_ex(serialized, CLIENT_INFO_TIMESTAMP_FIELD, &jtimestamp);
  ClientPingInfo inf;
  if (jtimestamp_exists) {
    inf.timestamp_ = json_object_get_int64(jtimestamp);
  }

  *this = inf;
  return common::Error();
}

timestamp_t ClientPingInfo::GetTimeStamp() const {
  return timestamp_;
}

}  // namespace fastotv
