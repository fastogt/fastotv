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

#include "commands_info/server_info.h"

#define BANDWIDTH_HOST_FIELD "bandwidth_host"

namespace fastotv {

ServerInfo::ServerInfo() : bandwidth_host_() {}

ServerInfo::ServerInfo(const common::net::HostAndPort& bandwidth_host) : bandwidth_host_(bandwidth_host) {}

common::Error ServerInfo::SerializeFields(json_object* deserialized) const {
  const std::string host_str = common::ConvertToString(bandwidth_host_);
  json_object_object_add(deserialized, BANDWIDTH_HOST_FIELD, json_object_new_string(host_str.c_str()));
  return common::Error();
}

common::Error ServerInfo::DoDeSerialize(json_object* serialized) {
  json_object* jband = nullptr;
  json_bool jband_exists = json_object_object_get_ex(serialized, BANDWIDTH_HOST_FIELD, &jband);
  ServerInfo inf;
  if (jband_exists) {
    const std::string host_str = json_object_get_string(jband);
    common::net::HostAndPort hs;
    if (common::ConvertFromString(host_str, &hs)) {
      inf.bandwidth_host_ = hs;
    }
  }

  *this = inf;
  return common::Error();
}

common::net::HostAndPort ServerInfo::GetBandwidthHost() const {
  return bandwidth_host_;
}

}  // namespace fastotv
