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

#include <common/sprintf.h>
#include <common/convert2string.h>

#define CHANNELS_FIELD "channels"

namespace fasto {
namespace fastotv {
namespace server {

UserInfo::UserInfo() : auth() {
}

UserInfo::UserInfo(const AuthInfo& a, const ChannelsInfo& ch) : auth(a), ch(ch) {
}

std::string UserInfo::GetLogin() const {
  return auth.GetLogin();
}

std::string UserInfo::GetPassword() const {
  return auth.GetPassword();
}

bool UserInfo::IsValid() const {
  return auth.IsValid();
}

common::Error UserInfo::Serialize(serialize_type* deserialized) const {
  json_object* obj = json_object_new_object();

  const std::string login_str = auth.GetLogin();
  const std::string password_str = auth.GetPassword();
  json_object_object_add(obj, AUTH_INFO_LOGIN_FIELD, json_object_new_string(login_str.c_str()));
  json_object_object_add(
      obj, AUTH_INFO_PASSWORD_FIELD, json_object_new_string(password_str.c_str()));

  json_object* jchannels = NULL;
  common::Error err = ch.Serialize(&jchannels);
  if (err && err->IsError()) {
    return err;
  }
  json_object_object_add(obj, CHANNELS_FIELD, jchannels);

  *deserialized = obj;
  return common::Error();
}

common::Error UserInfo::DeSerialize(const serialize_type& serialized, value_type* obj) {
  ChannelsInfo chan;
  json_object* jchan = NULL;
  json_bool jchan_exists = json_object_object_get_ex(serialized, CHANNELS_FIELD, &jchan);
  if (jchan_exists) {
    common::Error err = ChannelsInfo::DeSerialize(jchan, &chan);
    if (err && err->IsError()) {
      return err;
    }
  }

  AuthInfo ainf;
  common::Error err = AuthInfo::DeSerialize(serialized, &ainf);
  if (err && err->IsError()) {
    return err;
  }

  *obj = UserInfo(ainf, chan);
  return common::Error();
}
}
}  // namespace fastotv
}  // namespace fasto
