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

#include <common/net/types.h>
#include <common/error.h>

#include "infos.h"

namespace fasto {
namespace fastotv {
namespace server {

struct redis_configuration_t {
  common::net::HostAndPort redis_host;
  std::string redis_unix_socket;
};

struct redis_sub_configuration_t : public redis_configuration_t {
  std::string channel_in;
  std::string channel_out;
  std::string channel_clients_state;
};

class RedisStorage {
 public:
  RedisStorage();
  void SetConfig(const redis_configuration_t& config);

  common::Error FindUserAuth(const AuthInfo& user) const WARN_UNUSED_RESULT;  // check password
  common::Error FindUser(const AuthInfo& user,
                         UserInfo* uinf) const WARN_UNUSED_RESULT;  // check password

 private:
  redis_configuration_t config_;
};

class RedisSubHandler {
 public:
  virtual void handleMessage(const std::string& channel, const std::string& msg) = 0;
  virtual ~RedisSubHandler();
};

class RedisSub {
 public:
  explicit RedisSub(RedisSubHandler* handler);

  void SetConfig(const redis_sub_configuration_t& config);
  void Listen();
  void Stop();

  bool PublishStateToChannel(const std::string& msg) WARN_UNUSED_RESULT;
  bool PublishToChannelOut(const std::string& msg) WARN_UNUSED_RESULT;
  bool Publish(const char* chn, size_t chn_len, const char* msg, size_t msg_len) WARN_UNUSED_RESULT;

 private:
  RedisSubHandler* const handler_;
  redis_sub_configuration_t config_;
  bool stop_;
};

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
