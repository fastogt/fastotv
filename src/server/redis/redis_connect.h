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

#include <common/error.h>

#include "server/redis/redis_config.h"

struct redisContext;

namespace fastotv {
namespace server {
namespace redis {

common::Error redis_tcp_connect(const common::net::HostAndPort& host, redisContext** conn);
common::Error redis_unix_connect(const std::string& unix_path, redisContext** conn);

common::Error redis_connect(const RedisConfig& config, redisContext** conn);

}  // namespace redis
}  // namespace server
}  // namespace fastotv
