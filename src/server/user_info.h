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

#include <string>
#include <vector>

#include <common/net/types.h>
#include <common/smart_ptr.h>

#include "auth_info.h"
#include "channels_info.h"
#include "serializer/json_serializer.h"

namespace fasto {
namespace fastotv {
namespace server {

typedef std::string user_id_t;  // mongodb/redis id

struct UserInfo : public JsonSerializer<UserInfo> {
  UserInfo();
  explicit UserInfo(const AuthInfo& a, const ChannelsInfo& ch);

  bool IsValid() const;
  std::string GetLogin() const;
  std::string GetPassword() const;

  common::Error Serialize(serialize_type* deserialized) const WARN_UNUSED_RESULT;
  static common::Error DeSerialize(const serialize_type& serialized,
                                   value_type* obj) WARN_UNUSED_RESULT;
  AuthInfo auth;
  ChannelsInfo ch;
};

inline bool operator==(const UserInfo& lhs, const UserInfo& rhs) {
  return lhs.auth == rhs.auth;
}

inline bool operator!=(const UserInfo& x, const UserInfo& y) {
  return !(x == y);
}
}
}  // namespace fastotv
}  // namespace fasto
