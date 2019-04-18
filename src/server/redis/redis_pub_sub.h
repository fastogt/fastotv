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

#include <common/error.h>

#include "server/redis/redis_pub_sub_handler.h"
#include "server/redis/redis_sub_config.h"

namespace fastotv {
namespace server {
namespace redis {

class RedisPubSub {
 public:
  explicit RedisPubSub(RedisSubHandler* handler);

  void SetConfig(const RedisSubConfig& config);
  void Listen();
  void Stop();

  common::Error PublishStateToChannel(const std::string& msg) WARN_UNUSED_RESULT;
  common::Error PublishToChannelOut(const std::string& msg) WARN_UNUSED_RESULT;

 private:
  common::Error Publish(const std::string& channel, const std::string& msg) WARN_UNUSED_RESULT;

  RedisSubHandler* const handler_;
  RedisSubConfig config_;
  bool stop_;
};

}  // namespace redis
}  // namespace server
}  // namespace fastotv
