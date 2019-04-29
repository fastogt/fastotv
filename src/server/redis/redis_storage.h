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

#include <string>  // for string
#include <vector>

#include <common/error.h>      // for Error
#include <common/net/types.h>  // for HostAndPort

#include "commands_info/auth_info.h"
#include "server/user_info.h"

#include "server/redis/redis_config.h"

namespace fastotv {
namespace server {
namespace redis {

class RedisStorage {
 public:
  RedisStorage();
  void SetConfig(const RedisConfig& config);

  common::Error FindUser(const AuthInfo& user,
                         UserInfo* uinf) const WARN_UNUSED_RESULT;  // check password

  common::Error GetChatChannels(std::vector<stream_id>* channels) const;

 private:
  RedisConfig config_;
};

}  // namespace redis
}  // namespace server
}  // namespace fastotv
