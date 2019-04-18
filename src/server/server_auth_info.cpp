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

#include "server/server_auth_info.h"

#define SERVER_AUTH_INFO_UID_FIELD "id"

namespace fastotv {
namespace server {

ServerAuthInfo::ServerAuthInfo() : base_class() {}

ServerAuthInfo::ServerAuthInfo(const user_id_t& uid, const AuthInfo& auth) : base_class(auth), uid_(uid) {}

bool ServerAuthInfo::IsValid() const {
  return !uid_.empty() && base_class::IsValid();
}

common::Error ServerAuthInfo::SerializeFields(json_object* deserialized) const {
  if (!IsValid()) {
    return common::make_error_inval();
  }

  json_object_object_add(deserialized, SERVER_AUTH_INFO_UID_FIELD, json_object_new_string(uid_.c_str()));
  return base_class::SerializeFields(deserialized);
}

common::Error ServerAuthInfo::DoDeSerialize(json_object* serialized) {
  ServerAuthInfo inf;
  common::Error err = inf.base_class::DoDeSerialize(serialized);
  if (err) {
    return err;
  }

  json_object* jid = nullptr;
  json_bool jid_exists = json_object_object_get_ex(serialized, SERVER_AUTH_INFO_UID_FIELD, &jid);
  if (!jid_exists) {
    return common::make_error_inval();
  }

  inf.uid_ = json_object_get_string(jid);
  *this = inf;
  return common::Error();
}

user_id_t ServerAuthInfo::GetUserID() const {
  return uid_;
}

bool ServerAuthInfo::Equals(const ServerAuthInfo& auth) const {
  return uid_ == auth.uid_ && base_class::Equals(auth);
}

UserRpcInfo ServerAuthInfo::MakeUserRpc() const {
  return UserRpcInfo(uid_, GetDeviceID());
}

}  // namespace server
}  // namespace fastotv
