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

#define AUTH_FIELD "auth"
#define CHANNELS_FIELD "channels"

namespace fasto {
namespace fastotv {
namespace server {

UserInfo::UserInfo() : auth_() {
}

UserInfo::UserInfo(const AuthInfo& a, const ChannelsInfo& ch) : auth_(a), ch_(ch) {
}

bool UserInfo::IsValid() const {
  return auth_.IsValid();
}

common::Error UserInfo::Serialize(serialize_type* deserialized) const {
  json_object* obj = json_object_new_object();

  json_object* jauth = NULL;
  common::Error err = auth_.Serialize(&jauth);
  if (err && err->IsError()) {
    return err;
  }
  json_object_object_add(obj, AUTH_FIELD, jauth);

  json_object* jchannels = NULL;
  err = ch_.Serialize(&jchannels);
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
  json_object* jainf = NULL;
  json_bool jauth_exists = json_object_object_get_ex(serialized, AUTH_FIELD, &jainf);
  if (jauth_exists) {
    common::Error err = AuthInfo::DeSerialize(jainf, &ainf);
    if (err && err->IsError()) {
      return err;
    }
  }

  *obj = UserInfo(ainf, chan);
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
}
}  // namespace fastotv
}  // namespace fasto
