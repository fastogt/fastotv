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

#pragma once

#include <string>
#include <vector>

#include "commands_info/channels_info.h"  // for ChannelsInfo

namespace fastotv {
namespace server {

enum Status { BANNED = 0, ACTIVE = 1 };

class UserInfo : public common::serializer::JsonSerializer<UserInfo> {
 public:
  typedef std::vector<device_id_t> devices_t;

  UserInfo();
  explicit UserInfo(const user_id_t& uid,
                    const login_t& login,
                    const std::string& password,
                    const ChannelsInfo& ch,
                    const devices_t& devices,
                    Status state);

  bool IsValid() const;
  bool IsBanned() const;

  bool HaveDevice(device_id_t dev) const;
  devices_t GetDevices() const;
  login_t GetLogin() const;
  std::string GetPassword() const;
  ChannelsInfo GetChannelInfo() const;
  user_id_t GetUserID() const;

  bool Equals(const UserInfo& inf) const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* deserialized) const override;

 private:
  user_id_t uid_;
  login_t login_;         // unique
  std::string password_;  // hash
  ChannelsInfo ch_;
  devices_t devices_;
  Status status_;
};

inline bool operator==(const UserInfo& lhs, const UserInfo& rhs) {
  return lhs.Equals(rhs);
}

inline bool operator!=(const UserInfo& x, const UserInfo& y) {
  return !(x == y);
}
}  // namespace server
}  // namespace fastotv
