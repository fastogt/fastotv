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

#include "server/redis/redis_connect.h"

#include <stddef.h>  // for NULL
#include <string>    // for string

#include <hiredis/hiredis.h>  // for redisFree, redisContext

#include <common/logger.h>     // for COMPACT_LOG_ERROR, ERROR_LOG
#include <common/net/types.h>  // for HostAndPort

namespace fasto {
namespace fastotv {
namespace server {

redisContext* redis_connect(const RedisConfig& config) {
  const common::net::HostAndPort redisHost = config.redis_host;
  const std::string unixPath = config.redis_unix_socket;

  if (!redisHost.IsValid() && unixPath.empty()) {
    return NULL;
  }

  struct redisContext* redis = NULL;
  if (unixPath.empty()) {
    redis = redisConnect(redisHost.host.c_str(), redisHost.port);
  } else {
    redis = redisConnectUnix(unixPath.c_str());
    if (!redis || redis->err) {
      if (redis) {
        ERROR_LOG() << "REDIS UNIX CONNECTION ERROR: " << redis->errstr;
        redisFree(redis);
        redis = NULL;
      }
      redis = redisConnect(redisHost.host.c_str(), redisHost.port);
    }
  }

  if (redis->err) {
    ERROR_LOG() << "REDIS CONNECTION ERROR: " << redis->errstr;
    redisFree(redis);
    return NULL;
  }

  return redis;
}

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
