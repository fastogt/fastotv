/*  Copyright (C) 2014-2019 FastoGT. All right reserved.

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

#define USER_INFO_ID_FIELD "id"
#define USER_INFO_DEVICES_FIELD "devices"
#define USER_INFO_CHANNELS_FIELD "channels"
#define USER_INFO_LOGIN_FIELD "login"
#define USER_INFO_PASSWORD_FIELD "password"
#define USER_INFO_STATUS_FIELD "status"

namespace fastotv {
namespace server {

UserInfo::UserInfo() : login_(), password_(), ch_(), devices_(), status_(BANNED) {}

UserInfo::UserInfo(const user_id_t& uid,
                   const login_t& login,
                   const std::string& password,
                   const ChannelsInfo& ch,
                   const devices_t& devices,
                   Status state)
    : uid_(uid), login_(login), password_(password), ch_(ch), devices_(devices), status_(state) {}

bool UserInfo::IsValid() const {
  return !uid_.empty() && !login_.empty() && !password_.empty();
}

bool UserInfo::IsBanned() const {
  return status_ == BANNED;
}

common::Error UserInfo::SerializeFields(json_object* deserialized) const {
  if (!IsValid()) {
    return common::make_error_inval();
  }

  json_object_object_add(deserialized, USER_INFO_ID_FIELD, json_object_new_string(uid_.c_str()));
  json_object_object_add(deserialized, USER_INFO_LOGIN_FIELD, json_object_new_string(login_.c_str()));
  json_object_object_add(deserialized, USER_INFO_PASSWORD_FIELD, json_object_new_string(password_.c_str()));
  json_object_object_add(deserialized, USER_INFO_STATUS_FIELD, json_object_new_int(status_));

  json_object* jchannels = nullptr;
  common::Error err = ch_.Serialize(&jchannels);
  if (err) {
    return err;
  }
  json_object_object_add(deserialized, USER_INFO_CHANNELS_FIELD, jchannels);

  json_object* jdevices = json_object_new_array();
  for (size_t i = 0; i < devices_.size(); ++i) {
    json_object* jdevice = json_object_new_string(devices_[i].c_str());
    json_object_array_add(jdevices, jdevice);
  }
  json_object_object_add(deserialized, USER_INFO_DEVICES_FIELD, jdevices);
  return common::Error();
}

common::Error UserInfo::DoDeSerialize(json_object* serialized) {
  ChannelsInfo chan;
  json_object* jchan = nullptr;
  json_bool jchan_exists = json_object_object_get_ex(serialized, USER_INFO_CHANNELS_FIELD, &jchan);
  if (jchan_exists) {
    common::Error err = chan.DeSerialize(jchan);
    if (err) {
      return err;
    }
  }

  user_id_t uid;
  json_object* juid = nullptr;
  json_bool juid_exists = json_object_object_get_ex(serialized, USER_INFO_ID_FIELD, &juid);
  if (!juid_exists) {
    return common::make_error_inval();
  }
  uid = json_object_get_string(juid);

  std::string login;
  json_object* jlogin = nullptr;
  json_bool jlogin_exists = json_object_object_get_ex(serialized, USER_INFO_LOGIN_FIELD, &jlogin);
  if (!jlogin_exists) {
    return common::make_error_inval();
  }
  login = json_object_get_string(jlogin);

  std::string password;
  json_object* jpassword = nullptr;
  json_bool jpassword_exists = json_object_object_get_ex(serialized, USER_INFO_PASSWORD_FIELD, &jpassword);
  if (!jpassword_exists) {
    return common::make_error_inval();
  }
  password = json_object_get_string(jpassword);

  Status state;
  json_object* jstate = nullptr;
  json_bool jstate_exists = json_object_object_get_ex(serialized, USER_INFO_STATUS_FIELD, &jstate);
  if (!jstate_exists) {
    return common::make_error_inval();
  }
  state = static_cast<Status>(json_object_get_int(jstate));

  devices_t devices;
  json_object* jdevices = nullptr;
  json_bool jdevices_exists = json_object_object_get_ex(serialized, USER_INFO_DEVICES_FIELD, &jdevices);
  if (jdevices_exists) {
    size_t len = json_object_array_length(jdevices);
    for (size_t i = 0; i < len; ++i) {
      json_object* jdevice = json_object_array_get_idx(jdevices, i);
      devices.push_back(json_object_get_string(jdevice));
    }
  }

  *this = UserInfo(uid, login, password, chan, devices, state);
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

user_id_t UserInfo::GetUserID() const {
  return uid_;
}

bool UserInfo::Equals(const UserInfo& uinf) const {
  return uid_ == uinf.uid_ && login_ == uinf.login_ && password_ == uinf.password_ && ch_ == uinf.ch_;
}

}  // namespace server
}  // namespace fastotv
