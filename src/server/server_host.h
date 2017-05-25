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
#include <unordered_map>

#include <common/threads/types.h>

#include <common/libev/tcp/tcp_client.h>

#include "redis/redis_helpers.h"

#include "server/config.h"

namespace fasto {
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
  typedef std::unordered_map<user_id_t, inner::InnerTcpClient*> inner_connections_type;

  explicit ServerHost(const Config& config);
  ~ServerHost();

  void Stop();
  int Exec();

  common::Error UnRegisterInnerConnectionByHost(common::libev::IoClient* connection)
      WARN_UNUSED_RESULT;
  common::Error RegisterInnerConnectionByUser(user_id_t user_id,
                                              const AuthInfo& user,
                                              common::libev::IoClient* connection)
      WARN_UNUSED_RESULT;
  common::Error FindUserAuth(const AuthInfo& user, user_id_t* uid) const WARN_UNUSED_RESULT;
  common::Error FindUser(const AuthInfo& auth,
                         user_id_t* uid,
                         UserInfo* uinf) const WARN_UNUSED_RESULT;

  inner::InnerTcpClient* FindInnerConnectionByID(user_id_t user_id) const;
 private:
  DISALLOW_COPY_AND_ASSIGN(ServerHost);
  common::mutex stop_mutex_;
  common::condition_variable stop_cond_;
  bool stop_;

  inner::InnerTcpHandlerHost* handler_;
  inner::InnerTcpServer* server_;

  inner_connections_type connections_;
  RedisStorage rstorage_;
  const Config config_;
};

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
