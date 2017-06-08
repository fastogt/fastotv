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

#include <stddef.h>  // for NULL
#include <string>    // for string

#include "third-party/json-c/json-c/json_object.h"  // for json_object, json...

#define CHANNELS_FIELD "channels"
#define USER_INFO_LOGIN_FIELD "login"
#define USER_INFO_PASSWORD_FIELD "password"

namespace fasto {
namespace fastotv {
namespace server {

UserInfo::UserInfo() : auth_(), ch_() {
}

UserInfo::UserInfo(const AuthInfo& auth, const ChannelsInfo& ch) : auth_(auth), ch_(ch) {
}

bool UserInfo::IsValid() const {
  return auth_.IsValid();
}

common::Error UserInfo::SerializeImpl(serialize_type* deserialized) const {
  if (!IsValid()) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  json_object* obj = json_object_new_object();

  const std::string login = auth_.GetLogin();
  const std::string password = auth_.GetPassword();
  json_object_object_add(obj, USER_INFO_LOGIN_FIELD, json_object_new_string(login.c_str()));
  json_object_object_add(obj, USER_INFO_PASSWORD_FIELD, json_object_new_string(password.c_str()));

  json_object* jchannels = NULL;
  common::Error err = ch_.Serialize(&jchannels);
  if (err && err->IsError()) {
    return err;
  }
  json_object_object_add(obj, CHANNELS_FIELD, jchannels);

  *deserialized = obj;
  return common::Error();
}

common::Error UserInfo::DeSerialize(const serialize_type& serialized, value_type* obj) {
  if (!serialized || !obj) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  ChannelsInfo chan;
  json_object* jchan = NULL;
  json_bool jchan_exists = json_object_object_get_ex(serialized, CHANNELS_FIELD, &jchan);
  if (jchan_exists) {
    common::Error err = ChannelsInfo::DeSerialize(jchan, &chan);
    if (err && err->IsError()) {
      return err;
    }
  }

  std::string login;
  json_object* jlogin = NULL;
  json_bool jlogin_exists = json_object_object_get_ex(serialized, USER_INFO_LOGIN_FIELD, &jlogin);
  if (!jlogin_exists) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }
  login = json_object_get_string(jlogin);

  std::string password;
  json_object* jpassword = NULL;
  json_bool jpassword_exists = json_object_object_get_ex(serialized, USER_INFO_PASSWORD_FIELD, &jpassword);
  if (!jpassword_exists) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }
  password = json_object_get_string(jpassword);

  *obj = UserInfo(AuthInfo(login, password), chan);
  return common::Error();
}

AuthInfo UserInfo::GetAuthInfo() const {
  return auth_;
}

ChannelsInfo UserInfo::GetChannelInfo() const {
  return ch_;
}

bool UserInfo::Equals(const UserInfo& uinf) const {
  return auth_ == uinf.auth_ && ch_ == uinf.ch_;
}
}  // namespace server
}  // namespace fastotv
}  // namespace fasto
