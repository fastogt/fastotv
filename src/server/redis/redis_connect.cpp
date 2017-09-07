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

#include <common/error.h>
#include <common/net/types.h>  // for HostAndPort

namespace fastotv {
namespace server {
namespace redis {

common::Error redis_tcp_connect(const common::net::HostAndPort& host, redisContext** conn) {
  if (!conn || !host.IsValid()) {
    return common::make_error_inval(common::ERROR_TYPE);
  }

  const std::string host_str = host.GetHost();
  struct redisContext* redis = redisConnect(host_str.c_str(), host.GetPort());
  if (!redis || redis->err) {
    if (redis) {
      common::Error err = common::make_error(redis->errstr, common::ERROR_TYPE);
      redisFree(redis);
      redis = NULL;
      return err;
    }

    return common::make_error(
        common::MemSPrintf("Could not connect to Redis at %s:%u : no context", host_str, host.GetPort()),
        common::ERROR_TYPE);
  }

  *conn = redis;
  return common::Error();
}

common::Error redis_unix_connect(const std::string& unix_path, redisContext** conn) {
  if (!conn || unix_path.empty()) {
    return common::make_error_inval(common::ERROR_TYPE);
  }

  struct redisContext* redis = redisConnectUnix(unix_path.c_str());
  if (!redis || redis->err) {
    if (redis) {
      common::Error err = common::make_error(redis->errstr, common::ERROR_TYPE);
      redisFree(redis);
      redis = NULL;
      return err;
    }

    return common::make_error(common::MemSPrintf("Could not connect to Redis at %s : no context", unix_path),
                              common::ERROR_TYPE);
  }

  *conn = redis;
  return common::Error();
}

common::Error redis_connect(const RedisConfig& config, redisContext** conn) {
  if (!conn) {
    return common::make_error_inval(common::ERROR_TYPE);
  }

  const common::net::HostAndPort redis_host = config.redis_host;
  const std::string unix_path = config.redis_unix_socket;

  if (!redis_host.IsValid() && unix_path.empty()) {
    return common::make_error_inval(common::ERROR_TYPE);
  }

  if (unix_path.empty()) {
    struct redisContext* redis = NULL;
    common::Error err = redis_tcp_connect(redis_host, &redis);
    if (err) {
      return err;
    }

    *conn = redis;
    return common::Error();
  }

  struct redisContext* redis = NULL;
  common::Error err = redis_unix_connect(unix_path, &redis);
  if (err) {
    if (!redis_host.IsValid()) {
      return err;
    }

    common::Error tcp_err = redis_tcp_connect(redis_host, &redis);
    if (tcp_err) {
      return err;
    }
  }

  *conn = redis;
  return common::Error();
}

}  // namespace redis
}  // namespace server
}  // namespace fastotv
