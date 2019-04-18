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

#include <unordered_map>

#include <common/error.h>   // for Error
#include <common/macros.h>  // for WARN_UNUSED_RESULT, DISALLOW_COPY_...

#include "redis/redis_storage.h"

#include "server/config.h"  // for Config
#include "server/server_auth_info.h"

namespace fastotv {
namespace server {
namespace inner {
class InnerTcpClient;
class InnerTcpHandlerHost;
class InnerTcpServer;
}  // namespace inner

class ServerHost {
 public:
  enum { timeout_seconds = 1 };
  typedef inner::InnerTcpClient client_t;
  typedef std::unordered_map<user_id_t, std::vector<client_t*>> inner_connections_t;

  explicit ServerHost(const Config& config);
  ~ServerHost();

  void Stop();
  int Exec();

  common::Error UnRegisterInnerConnectionByHost(client_t* client) WARN_UNUSED_RESULT;
  common::Error RegisterInnerConnectionByUser(const ServerAuthInfo& user, client_t* client) WARN_UNUSED_RESULT;
  common::Error FindUser(const AuthInfo& auth, UserInfo* uinf) const WARN_UNUSED_RESULT;

  common::Error GetChatChannels(std::vector<stream_id>* channels) const WARN_UNUSED_RESULT;

  inner::InnerTcpClient* FindInnerConnectionByUser(const rpc::UserRpcInfo& user) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(ServerHost);

  inner::InnerTcpHandlerHost* handler_;
  inner::InnerTcpServer* server_;

  inner_connections_t connections_;
  redis::RedisStorage rstorage_;
  const Config config_;
};

}  // namespace server
}  // namespace fastotv
