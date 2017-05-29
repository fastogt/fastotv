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

#include "server/user_info.h"

#include <string>

#include "third-party/json-c/json-c/json.h"  // for json_object_...

#include <common/sprintf.h>
#include <common/convert2string.h>

#define CHANNELS_FIELD "channels"

namespace fasto {
namespace fastotv {
namespace server {

UserInfo::UserInfo() : auth() {
}

UserInfo::UserInfo(const AuthInfo& a, const channels_t& ch) : auth(a), channels(ch) {
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

json_object* UserInfo::MakeJobject(const UserInfo& uinfo) {
  json_object* obj = json_object_new_object();

  fasto::fastotv::AuthInfo ainfo = uinfo.auth;
  const std::string login_str = ainfo.login;
  const std::string password_str = ainfo.password;
  json_object_object_add(obj, AUTH_INFO_LOGIN_FIELD, json_object_new_string(login_str.c_str()));
  json_object_object_add(
      obj, AUTH_INFO_PASSWORD_FIELD, json_object_new_string(password_str.c_str()));

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
}
}  // namespace fastotv
}  // namespace fasto
