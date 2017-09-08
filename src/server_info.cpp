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

#include "server_info.h"

#include <string>

#include <common/convert2string.h>

namespace fastotv {

ServerInfo::ServerInfo() : bandwidth_host_() {}

ServerInfo::ServerInfo(const common::net::HostAndPort& bandwidth_host) : bandwidth_host_(bandwidth_host) {}

common::Error ServerInfo::SerializeImpl(serialize_type* deserialized) const {
  json_object* obj = json_object_new_object();
  const std::string host_str = common::ConvertToString(bandwidth_host_);
  json_object_object_add(obj, BANDWIDTH_HOST_FIELD, json_object_new_string(host_str.c_str()));
  *deserialized = obj;
  return common::Error();
}

common::Error ServerInfo::DeSerialize(const serialize_type& serialized, value_type* obj) {
  if (!serialized || !obj) {
    return common::make_error_inval();
  }

  json_object* jband = NULL;
  json_bool jband_exists = json_object_object_get_ex(serialized, BANDWIDTH_HOST_FIELD, &jband);
  ServerInfo inf;
  if (jband_exists) {
    const std::string host_str = json_object_get_string(jband);
    common::net::HostAndPort hs;
    if (common::ConvertFromString(host_str, &hs)) {
      inf.bandwidth_host_ = hs;
    }
  }
  *obj = inf;
  return common::Error();
}

common::net::HostAndPort ServerInfo::GetBandwidthHost() const {
  return bandwidth_host_;
}

}  // namespace fastotv
