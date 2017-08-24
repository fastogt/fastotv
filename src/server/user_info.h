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

#include "channels_info.h"  // for ChannelsInfo

#include "serializer/json_serializer.h"

namespace fastotv {
namespace server {

typedef std::string user_id_t;  // mongodb/redis id

class UserInfo : public JsonSerializer<UserInfo> {
 public:
  typedef std::vector<device_id_t> devices_t;

  UserInfo();
  explicit UserInfo(const login_t& login,
                    const std::string& password,
                    const ChannelsInfo& ch,
                    const devices_t& devices);

  bool IsValid() const;

  static common::Error DeSerialize(const serialize_type& serialized, value_type* obj) WARN_UNUSED_RESULT;

  bool HaveDevice(device_id_t dev) const;
  devices_t GetDevices() const;
  login_t GetLogin() const;
  std::string GetPassword() const;
  ChannelsInfo GetChannelInfo() const;

  bool Equals(const UserInfo& inf) const;

 protected:
  virtual common::Error SerializeImpl(serialize_type* deserialized) const override;

 private:
  login_t login_;  // unique
  std::string password_;
  ChannelsInfo ch_;
  devices_t devices_;
};

inline bool operator==(const UserInfo& lhs, const UserInfo& rhs) {
  return lhs.Equals(rhs);
}

inline bool operator!=(const UserInfo& x, const UserInfo& y) {
  return !(x == y);
}
}  // namespace server
}  // namespace fastotv
