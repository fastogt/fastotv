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

#include <string>

#include <common/serializer/json_serializer.h>

#include "client_server_types.h"  // for login_t

namespace fastotv {

class AuthInfo : public common::serializer::JsonSerializer<AuthInfo> {
 public:
  AuthInfo();
  AuthInfo(const login_t& login, const std::string& password, device_id_t dev);

  bool IsValid() const;

  device_id_t GetDeviceID() const;
  login_t GetLogin() const;
  std::string GetPassword() const;
  bool Equals(const AuthInfo& auth) const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* deserialized) const override;

 private:
  login_t login_;  // unique
  std::string password_;
  device_id_t device_id_;
};

inline bool operator==(const AuthInfo& lhs, const AuthInfo& rhs) {
  return lhs.Equals(rhs);
}

inline bool operator!=(const AuthInfo& x, const AuthInfo& y) {
  return !(x == y);
}

}  // namespace fastotv
