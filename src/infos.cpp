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

#include "infos.h"

#include <string>

#include "third-party/json-c/json-c/json.h"  // for json_object_...

#include <common/sprintf.h>

#define LOGIN_FIELD "login"
#define PASSWORD_FIELD "password"

#define CHANNELS_FIELD "channels"

namespace fasto {
namespace fastotv {

ConnectInfo::ConnectInfo() {
}

ConnectInfo::ConnectInfo(const common::net::HostAndPort& host) : host(host) {
}

AuthInfo::AuthInfo() : login(), password() {
}

AuthInfo::AuthInfo(const std::string& login, const std::string& password)
    : login(login), password(password) {
}

bool AuthInfo::IsValid() const {
  return !login.empty();
}

struct json_object* AuthInfo::MakeJobject(const AuthInfo& ainf) {
  json_object* obj = json_object_new_object();
  const std::string login_str = ainf.login;
  const std::string password_str = ainf.password;
  json_object_object_add(obj, LOGIN_FIELD, json_object_new_string(login_str.c_str()));
  json_object_object_add(obj, PASSWORD_FIELD, json_object_new_string(password_str.c_str()));
  return obj;
}

AuthInfo AuthInfo::MakeClass(json_object* obj) {
  json_object* jlogin = NULL;
  json_bool jlogin_exists = json_object_object_get_ex(obj, LOGIN_FIELD, &jlogin);
  if (!jlogin_exists) {
    return fasto::fastotv::AuthInfo();
  }

  json_object* jpass = NULL;
  json_bool jpass_exists = json_object_object_get_ex(obj, PASSWORD_FIELD, &jpass);
  if (!jpass_exists) {
    return fasto::fastotv::AuthInfo();
  }

  fasto::fastotv::AuthInfo ainf(json_object_get_string(jlogin), json_object_get_string(jpass));
  return ainf;
}

UserInfo::UserInfo() : auth() {
}

UserInfo::UserInfo(const AuthInfo& a, const std::vector<Url>& ch) : auth(a), channels(ch) {
}

std::string UserInfo::GetLogin() const {
  return auth.login;
}

std::string UserInfo::GetPassword() const {
  return auth.password;
}

bool UserInfo::IsValid() const {
  return auth.IsValid();
}

json_object* MakeJobjectFromChannels(const channels_t& channels) {
  json_object* jchannels = json_object_new_array();
  for (Url url : channels) {
    json_object_array_add(jchannels, Url::MakeJobject(url));
  }
  return jchannels;
}

channels_t MakeChannelsClass(json_object* obj) {
  channels_t chan;
  int len = json_object_array_length(obj);
  for (int i = 0; i < len; ++i) {
    chan.push_back(Url::MakeClass(json_object_array_get_idx(obj, i)));
  }
  return chan;
}

json_object* UserInfo::MakeJobject(const UserInfo& uinfo) {
  json_object* obj = json_object_new_object();

  fasto::fastotv::AuthInfo ainfo = uinfo.auth;
  const std::string login_str = ainfo.login;
  const std::string password_str = ainfo.password;
  json_object_object_add(obj, LOGIN_FIELD, json_object_new_string(login_str.c_str()));
  json_object_object_add(obj, PASSWORD_FIELD, json_object_new_string(password_str.c_str()));

  json_object* jchannels = MakeJobjectFromChannels(uinfo.channels);
  json_object_object_add(obj, CHANNELS_FIELD, jchannels);
  return obj;
}

UserInfo UserInfo::MakeClass(json_object* obj) {
  channels_t chan;
  json_object* jchan = NULL;
  json_bool jchan_exists = json_object_object_get_ex(obj, CHANNELS_FIELD, &jchan);
  if (jchan_exists) {
    int len = json_object_array_length(jchan);
    for (int i = 0; i < len; ++i) {
      chan.push_back(Url::MakeClass(json_object_array_get_idx(jchan, i)));
    }
  }
  return UserInfo(AuthInfo::MakeClass(obj), chan);
}

}  // namespace fastotv
}  // namespace fasto
