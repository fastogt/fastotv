/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

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

#include <json-c/json_object.h>  // for json_object, json...

#define USER_INFO_DEVICES_FIELD "devices"
#define USER_INFO_CHANNELS_FIELD "channels"
#define USER_INFO_LOGIN_FIELD "login"
#define USER_INFO_PASSWORD_FIELD "password"

namespace fastotv {
namespace server {

UserInfo::UserInfo() : login_(), password_(), ch_() {}

UserInfo::UserInfo(const login_t& login, const std::string& password, const ChannelsInfo& ch, const devices_t& devices)
    : login_(login), password_(password), ch_(ch), devices_(devices) {}

bool UserInfo::IsValid() const {
  return !login_.empty() && !password_.empty();
}

common::Error UserInfo::SerializeFields(json_object* obj) const {
  if (!IsValid()) {
    return common::make_error_inval();
  }

  json_object_object_add(obj, USER_INFO_LOGIN_FIELD, json_object_new_string(login_.c_str()));
  json_object_object_add(obj, USER_INFO_PASSWORD_FIELD, json_object_new_string(password_.c_str()));

  json_object* jchannels = NULL;
  common::Error err = ch_.Serialize(&jchannels);
  if (err) {
    return err;
  }
  json_object_object_add(obj, USER_INFO_CHANNELS_FIELD, jchannels);

  json_object* jdevices = json_object_new_array();
  for (size_t i = 0; i < devices_.size(); ++i) {
    json_object* jdevice = json_object_new_string(devices_[i].c_str());
    json_object_array_add(jdevices, jdevice);
  }
  json_object_object_add(obj, USER_INFO_DEVICES_FIELD, jdevices);
  return common::Error();
}

common::Error UserInfo::DeSerialize(const serialize_type& serialized, UserInfo* obj) {
  if (!serialized || !obj) {
    return common::make_error_inval();
  }

  ChannelsInfo chan;
  json_object* jchan = NULL;
  json_bool jchan_exists = json_object_object_get_ex(serialized, USER_INFO_CHANNELS_FIELD, &jchan);
  if (jchan_exists) {
    common::Error err = ChannelsInfo::DeSerialize(jchan, &chan);
    if (err) {
      return err;
    }
  }

  std::string login;
  json_object* jlogin = NULL;
  json_bool jlogin_exists = json_object_object_get_ex(serialized, USER_INFO_LOGIN_FIELD, &jlogin);
  if (!jlogin_exists) {
    return common::make_error_inval();
  }
  login = json_object_get_string(jlogin);

  std::string password;
  json_object* jpassword = NULL;
  json_bool jpassword_exists = json_object_object_get_ex(serialized, USER_INFO_PASSWORD_FIELD, &jpassword);
  if (!jpassword_exists) {
    return common::make_error_inval();
  }
  password = json_object_get_string(jpassword);

  devices_t devices;
  json_object* jdevices = NULL;
  json_bool jdevices_exists = json_object_object_get_ex(serialized, USER_INFO_DEVICES_FIELD, &jdevices);
  if (jdevices_exists) {
    size_t len = json_object_array_length(jdevices);
    for (size_t i = 0; i < len; ++i) {
      json_object* jdevice = json_object_array_get_idx(jdevices, i);
      devices.push_back(json_object_get_string(jdevice));
    }
  }
  *obj = UserInfo(login, password, chan, devices);
  return common::Error();
}

bool UserInfo::HaveDevice(device_id_t dev) const {
  for (size_t i = 0; i < devices_.size(); ++i) {
    if (dev == devices_[i]) {
      return true;
    }
  }

  return false;
}

UserInfo::devices_t UserInfo::GetDevices() const {
  return devices_;
}

login_t UserInfo::GetLogin() const {
  return login_;
}

std::string UserInfo::GetPassword() const {
  return password_;
}

ChannelsInfo UserInfo::GetChannelInfo() const {
  return ch_;
}

bool UserInfo::Equals(const UserInfo& uinf) const {
  return login_ == uinf.login_ && password_ == uinf.password_ && ch_ == uinf.ch_;
}

}  // namespace server
}  // namespace fastotv
