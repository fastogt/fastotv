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

#include <common/error.h>      // for Error
#include <common/net/types.h>  // for HostAndPort

#include "server/user_info.h"  // for user_id_t, UserInfo (ptr only)

#include "server/redis/redis_config.h"

namespace fasto {
namespace fastotv {
namespace server {

class RedisStorage {
 public:
  RedisStorage();
  void SetConfig(const RedisConfig& config);

  common::Error FindUserAuth(const AuthInfo& user, user_id_t* uid) const WARN_UNUSED_RESULT;  // check password
  common::Error FindUser(const AuthInfo& user,
                         user_id_t* uid,
                         UserInfo* uinf) const WARN_UNUSED_RESULT;  // check password

 private:
  RedisConfig config_;
};

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
