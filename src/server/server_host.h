/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of SiteOnYourDevice.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>
#include <unordered_map>

#include <common/threads/types.h>

#include "network/tcp/tcp_client.h"

#include "redis/redis_helpers.h"

namespace fasto {
namespace fastotv {
namespace server {

namespace inner {
class InnerTcpClient;
class InnerTcpHandlerHost;
class InnerTcpServer;
}  // namespace inner

struct Settings {
  redis_sub_configuration_t redis;
};

struct Config {
  Settings server;
};

class ServerHost {
 public:
  enum { timeout_seconds = 1 };
  typedef std::unordered_map<std::string, inner::InnerTcpClient*> inner_connections_type;

  explicit ServerHost(const common::net::HostAndPort& host);
  ~ServerHost();

  void stop();
  int exec();

  bool unRegisterInnerConnectionByHost(tcp::TcpClient* connection) WARN_UNUSED_RESULT;
  bool registerInnerConnectionByUser(const AuthInfo& user,
                                     tcp::TcpClient* connection) WARN_UNUSED_RESULT;
  common::Error findUserAuth(const AuthInfo& user) const WARN_UNUSED_RESULT;
  common::Error findUser(const AuthInfo& auth, UserInfo* uinf) const WARN_UNUSED_RESULT;

  inner::InnerTcpClient* findInnerConnectionByLogin(const std::string& login) const;
  void setConfig(const Config& conf);

 private:
  std::mutex stop_mutex_;
  std::condition_variable stop_cond_;
  bool stop_;

  inner::InnerTcpHandlerHost* handler_;
  inner::InnerTcpServer* server_;

  inner_connections_type connections_;
  RedisStorage rstorage_;
};

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
