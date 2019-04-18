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

#include "server/user_rpc/user_rpc_info.h"

#define USER_STATE_INFO_USER_ID_FIELD "user_id"
#define USER_STATE_INFO_DEVICE_ID_FIELD "device_id"

namespace fastotv {
namespace server {

UserRpcInfo::UserRpcInfo() : user_id_(), device_id_() {}

UserRpcInfo::UserRpcInfo(const user_id_t& uid, const device_id_t& device_id) : user_id_(uid), device_id_(device_id) {}

bool UserRpcInfo::IsValid() const {
  return !user_id_.empty() && !device_id_.empty();
}

device_id_t UserRpcInfo::GetDeviceID() const {
  return device_id_;
}

user_id_t UserRpcInfo::GetUserID() const {
  return user_id_;
}

bool UserRpcInfo::Equals(const UserRpcInfo& state) const {
  return user_id_ == state.user_id_ && device_id_ == state.device_id_;
}

common::Error UserRpcInfo::SerializeFields(json_object* deserialized) const {
  json_object_object_add(deserialized, USER_STATE_INFO_USER_ID_FIELD, json_object_new_string(user_id_.c_str()));
  json_object_object_add(deserialized, USER_STATE_INFO_DEVICE_ID_FIELD, json_object_new_string(device_id_.c_str()));
  return common::Error();
}

common::Error UserRpcInfo::DoDeSerialize(json_object* serialized) {
  UserRpcInfo inf;
  json_object* jid = nullptr;
  json_bool jid_exists = json_object_object_get_ex(serialized, USER_STATE_INFO_USER_ID_FIELD, &jid);
  if (jid_exists) {
    inf.user_id_ = json_object_get_string(jid);
  }

  json_object* jdevice = nullptr;
  json_bool jdevice_exists = json_object_object_get_ex(serialized, USER_STATE_INFO_DEVICE_ID_FIELD, &jdevice);
  if (jdevice_exists) {
    inf.device_id_ = json_object_get_string(jdevice);
  }

  *this = inf;
  return common::Error();
}

}  // namespace server
}  // namespace fastotv
