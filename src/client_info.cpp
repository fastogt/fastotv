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

ClientInfo::ClientInfo() : login(), os(), cpu_brand(), ram_total(0), ram_free(0), bandwidth(0) {
}

bool ClientInfo::IsValid() const {
  return !login.empty();
}

struct json_object* ClientInfo::MakeJobject(const ClientInfo& inf) {
  json_object* obj = json_object_new_object();
  json_object_object_add(obj, CLIENT_INFO_LOGIN_FIELD, json_object_new_string(inf.login.c_str()));
  json_object_object_add(obj, CLIENT_INFO_OS_FIELD, json_object_new_string(inf.os.c_str()));
  json_object_object_add(obj, CLIENT_INFO_CPU_FIELD, json_object_new_string(inf.cpu_brand.c_str()));
  json_object_object_add(obj, CLIENT_INFO_RAM_TOTAL_FIELD, json_object_new_int64(inf.ram_total));
  json_object_object_add(obj, CLIENT_INFO_RAM_FREE_FIELD, json_object_new_int64(inf.ram_free));
  json_object_object_add(obj, CLIENT_INFO_BANDWIDTH_FIELD, json_object_new_int64(inf.bandwidth));
  return obj;
}

ClientInfo ClientInfo::MakeClass(json_object* obj) {
  ClientInfo inf;

  json_object* jlogin = NULL;
  json_bool jlogin_exists = json_object_object_get_ex(obj, CLIENT_INFO_BANDWIDTH_FIELD, &jlogin);
  if (jlogin_exists) {
    inf.login = json_object_get_string(jlogin);
  }

  json_object* jos = NULL;
  json_bool jos_exists = json_object_object_get_ex(obj, CLIENT_INFO_OS_FIELD, &jos);
  if (jos_exists) {
    inf.os = json_object_get_string(jos);
  }

  json_object* jcpu = NULL;
  json_bool jcpu_exists = json_object_object_get_ex(obj, CLIENT_INFO_CPU_FIELD, &jcpu);
  if (jcpu_exists) {
    inf.cpu_brand = json_object_get_string(jcpu);
  }

  json_object* jram_total = NULL;
  json_bool jram_total_exists =
      json_object_object_get_ex(obj, CLIENT_INFO_RAM_TOTAL_FIELD, &jram_total);
  if (jram_total_exists) {
    inf.ram_total = json_object_get_int64(jram_total);
  }

  json_object* jram_free = NULL;
  json_bool jram_free_exists =
      json_object_object_get_ex(obj, CLIENT_INFO_RAM_TOTAL_FIELD, &jram_free);
  if (jram_free_exists) {
    inf.ram_free = json_object_get_int64(jram_free);
  }

  json_object* jband = NULL;
  json_bool jband_exists = json_object_object_get_ex(obj, CLIENT_INFO_BANDWIDTH_FIELD, &jband);
  if (jband_exists) {
    inf.bandwidth = json_object_get_int64(jband);
  }
  return inf;
}

}  // namespace fastotv
}  // namespace fasto
