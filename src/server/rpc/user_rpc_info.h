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

#pragma once

#include <common/serializer/json_serializer.h>

#include "client_server_types.h"

namespace fastotv {
namespace server {
namespace rpc {

class UserRpcInfo : public common::serializer::JsonSerializer<UserRpcInfo> {
 public:
  UserRpcInfo();
  UserRpcInfo(const user_id_t& uid, const device_id_t& device_id);

  bool IsValid() const;

  device_id_t GetDeviceID() const;
  user_id_t GetUserID() const;

  bool Equals(const UserRpcInfo& state) const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* deserialized) const override;

 private:
  user_id_t user_id_;
  device_id_t device_id_;
};

inline bool operator==(const UserRpcInfo& lhs, const UserRpcInfo& rhs) {
  return lhs.Equals(rhs);
}

inline bool operator!=(const UserRpcInfo& x, const UserRpcInfo& y) {
  return !(x == y);
}

}  // namespace rpc
}  // namespace server
}  // namespace fastotv
