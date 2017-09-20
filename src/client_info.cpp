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

#define CLIENT_INFO_LOGIN_FIELD "login"
#define CLIENT_INFO_BANDWIDTH_FIELD "bandwidth"
#define CLIENT_INFO_OS_FIELD "os"
#define CLIENT_INFO_CPU_FIELD "cpu"
#define CLIENT_INFO_RAM_TOTAL_FIELD "ram_total"
#define CLIENT_INFO_RAM_FREE_FIELD "ram_free"

namespace fastotv {

ClientInfo::ClientInfo() : login_(), os_(), cpu_brand_(), ram_total_(0), ram_free_(0), bandwidth_(0) {}

ClientInfo::ClientInfo(const login_t& login,
                       const std::string& os,
                       const std::string& cpu_brand,
                       int64_t ram_total,
                       int64_t ram_free,
                       bandwidth_t bandwidth)
    : login_(login),
      os_(os),
      cpu_brand_(cpu_brand),
      ram_total_(ram_total),
      ram_free_(ram_free),
      bandwidth_(bandwidth) {}

bool ClientInfo::IsValid() const {
  return !login_.empty();
}

common::Error ClientInfo::SerializeImpl(serialize_type* deserialized) const {
  if (!IsValid()) {
    return common::make_error_inval();
  }

  json_object* obj = json_object_new_object();
  json_object_object_add(obj, CLIENT_INFO_LOGIN_FIELD, json_object_new_string(login_.c_str()));
  json_object_object_add(obj, CLIENT_INFO_OS_FIELD, json_object_new_string(os_.c_str()));
  json_object_object_add(obj, CLIENT_INFO_CPU_FIELD, json_object_new_string(cpu_brand_.c_str()));
  json_object_object_add(obj, CLIENT_INFO_RAM_TOTAL_FIELD, json_object_new_int64(ram_total_));
  json_object_object_add(obj, CLIENT_INFO_RAM_FREE_FIELD, json_object_new_int64(ram_free_));
  json_object_object_add(obj, CLIENT_INFO_BANDWIDTH_FIELD, json_object_new_int64(bandwidth_));
  *deserialized = obj;
  return common::Error();
}

common::Error ClientInfo::DeSerialize(const serialize_type& serialized, value_type* obj) {
  if (!serialized || !obj) {
    return common::make_error_inval();
  }

  ClientInfo inf;
  json_object* jlogin = NULL;
  json_bool jlogin_exists = json_object_object_get_ex(serialized, CLIENT_INFO_LOGIN_FIELD, &jlogin);
  if (!jlogin_exists) {
    return common::make_error_inval();
  }

  std::string login = json_object_get_string(jlogin);
  if (login.empty()) {
    return common::make_error_inval();
  }

  inf.login_ = login;

  json_object* jos = NULL;
  json_bool jos_exists = json_object_object_get_ex(serialized, CLIENT_INFO_OS_FIELD, &jos);
  if (jos_exists) {
    inf.os_ = json_object_get_string(jos);
  }

  json_object* jcpu = NULL;
  json_bool jcpu_exists = json_object_object_get_ex(serialized, CLIENT_INFO_CPU_FIELD, &jcpu);
  if (jcpu_exists) {
    inf.cpu_brand_ = json_object_get_string(jcpu);
  }

  json_object* jram_total = NULL;
  json_bool jram_total_exists = json_object_object_get_ex(serialized, CLIENT_INFO_RAM_TOTAL_FIELD, &jram_total);
  if (jram_total_exists) {
    inf.ram_total_ = json_object_get_int64(jram_total);
  }

  json_object* jram_free = NULL;
  json_bool jram_free_exists = json_object_object_get_ex(serialized, CLIENT_INFO_RAM_FREE_FIELD, &jram_free);
  if (jram_free_exists) {
    inf.ram_free_ = json_object_get_int64(jram_free);
  }

  json_object* jband = NULL;
  json_bool jband_exists = json_object_object_get_ex(serialized, CLIENT_INFO_BANDWIDTH_FIELD, &jband);
  if (jband_exists) {
    inf.bandwidth_ = json_object_get_int64(jband);
  }

  *obj = inf;
  return common::Error();
}

login_t ClientInfo::GetLogin() const {
  return login_;
}

std::string ClientInfo::GetOs() const {
  return os_;
}

std::string ClientInfo::GetCpuBrand() const {
  return cpu_brand_;
}

int64_t ClientInfo::GetRamTotal() const {
  return ram_total_;
}

int64_t ClientInfo::GetRamFree() const {
  return ram_free_;
}

bandwidth_t ClientInfo::GetBandwidth() const {
  return bandwidth_;
}

}  // namespace fastotv
