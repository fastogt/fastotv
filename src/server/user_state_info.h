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

#pragma once

#include "serializer/json_serializer.h"

#include <common/error.h>   // for Error
#include <common/macros.h>  // for WARN_UNUSED_RESULT

#include "server/user_info.h"  // for user_id_t

#define USER_STATE_INFO_USER_ID_FIELD "user_id"
#define USER_STATE_INFO_DEVICE_ID_FIELD "device_id"
#define USER_STATE_INFO_CONNECTED_FIELD "connected"

namespace fastotv {
namespace server {

class UserStateInfo : public JsonSerializer<UserStateInfo> {
 public:
  UserStateInfo();
  UserStateInfo(const user_id_t& uid, const device_id_t& device_id, bool connected);

  static common::Error DeSerialize(const serialize_type& serialized, value_type* obj) WARN_UNUSED_RESULT;

  device_id_t GetDeviceId() const;
  user_id_t GetUserId() const;
  bool IsConnected() const;

  bool Equals(const UserStateInfo& state) const;

 protected:
  virtual common::Error SerializeImpl(serialize_type* deserialized) const override WARN_UNUSED_RESULT;

 private:
  user_id_t user_id_;
  device_id_t device_id_;
  bool connected_;
};

inline bool operator==(const UserStateInfo& lhs, const UserStateInfo& rhs) {
  return lhs.Equals(rhs);
}

inline bool operator!=(const UserStateInfo& x, const UserStateInfo& y) {
  return !(x == y);
}
}  // namespace server
}  // namespace fastotv
