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

#include <string>  // for string

#include <common/error.h>   // for Error
#include <common/macros.h>  // for WARN_UNUSED_RESULT

#include "client_server_types.h"  // for login_t

#include "serializer/json_serializer.h"

namespace fasto {
namespace fastotv {

class AuthInfo : public JsonSerializer<AuthInfo> {
 public:
  AuthInfo();
  AuthInfo(const std::string& login, const std::string& password);

  bool IsValid() const;

  static common::Error DeSerialize(const serialize_type& serialized, value_type* obj) WARN_UNUSED_RESULT;

  login_t GetLogin() const;
  std::string GetPassword() const;
  bool Equals(const AuthInfo& auth) const;

 protected:
  common::Error SerializeImpl(serialize_type* deserialized) const override;

 private:
  login_t login_;  // unique
  std::string password_;
};

inline bool operator==(const AuthInfo& lhs, const AuthInfo& rhs) {
  return lhs.Equals(rhs);
}

inline bool operator!=(const AuthInfo& x, const AuthInfo& y) {
  return !(x == y);
}

}  // namespace fastotv
}  // namespace fasto
