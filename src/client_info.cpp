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

#include "client_info.h"

#include <string>

#include "third-party/json-c/json-c/json.h"  // for json_object_...

#include <common/convert2string.h>

namespace fasto {
namespace fastotv {

ClientInfo::ClientInfo() : bandwidth(0) {}

struct json_object* ClientInfo::MakeJobject(const ClientInfo& inf) {
  json_object* obj = json_object_new_object();
  json_object_object_add(obj, BANDWIDTH_FIELD, json_object_new_int64(inf.bandwidth));
  return obj;
}

ClientInfo ClientInfo::MakeClass(json_object* obj) {
  json_object* jband = NULL;
  json_bool jband_exists = json_object_object_get_ex(obj, BANDWIDTH_FIELD, &jband);
  ClientInfo inf;
  if (jband_exists) {
    inf.bandwidth = json_object_get_int64(jband);
  }
  return inf;
}

}  // namespace fastotv
}  // namespace fasto
