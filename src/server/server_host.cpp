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

#include "server/server_host.h"

#include <stdlib.h>  // for EXIT_FAILURE

#include <string>  // for string

#include <common/libev/tcp/tcp_server.h>    // for TcpServer
#include <common/logger.h>                  // for COMPACT_LOG_FILE_CRIT
#include <common/threads/thread_manager.h>  // for THREAD_MANAGER

#include "inner/inner_tcp_client.h"  // for InnerTcpClient

#include "server/inner/inner_tcp_handler.h"  // for InnerTcpHandlerHost
#include "server/inner/inner_tcp_server.h"

#define BUF_SIZE 4096
#define UNKNOWN_CLIENT_NAME "Unknown"

namespace fastotv {
namespace server {

ServerHost::ServerHost(const Config& config) : handler_(nullptr), server_(nullptr), rstorage_(), config_(config) {
  handler_ = new inner::InnerTcpHandlerHost(this, config);
  server_ = new inner::InnerTcpServer(config.server.host, true, handler_);
  server_->SetName("inner_server");

  rstorage_.SetConfig(config.server.redis);
}

ServerHost::~ServerHost() {
  destroy(&server_);
  destroy(&handler_);
}

void ServerHost::Stop() {
  server_->Stop();
}

int ServerHost::Exec() {
  common::ErrnoError err = server_->Bind(true);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    return EXIT_FAILURE;
  }

  err = server_->Listen(5);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    return EXIT_FAILURE;
  }

  return server_->Exec();
}

common::Error ServerHost::UnRegisterInnerConnectionByHost(client_t* client) {
  if (!client) {
    DNOTREACHED();
    return common::make_error_inval();
  }

  const auto sinf = client->GetServerHostInfo();
  if (!sinf.IsValid()) {
    return common::make_error_inval();
  }

  auto hs = connections_.find(sinf.GetUserID());
  if (hs == connections_.end()) {
    return common::Error();
  }

  for (auto it = hs->second.begin(); it != hs->second.end(); ++it) {
    if (*it == client) {
      hs->second.erase(it);
      break;
    }
  }

  if (hs->second.empty()) {
    connections_.erase(hs);
  }
  return common::Error();
}

common::Error ServerHost::RegisterInnerConnectionByUser(const ServerAuthInfo& user, client_t* client) {
  CHECK(user.IsValid());
  if (!client) {
    DNOTREACHED();
    return common::make_error_inval();
  }

  client->SetServerHostInfo(user);
  connections_[user.GetUserID()].push_back(client);
  client->SetName(user.GetLogin());
  return common::Error();
}

common::Error ServerHost::FindUser(const AuthInfo& auth, UserInfo* uinf) const {
  return rstorage_.FindUser(auth, uinf);
}

common::Error ServerHost::GetChatChannels(std::vector<stream_id>* channels) const {
  return rstorage_.GetChatChannels(channels);
}

inner::InnerTcpClient* ServerHost::FindInnerConnectionByUser(const rpc::UserRpcInfo& user) const {
  const auto hs = connections_.find(user.GetUserID());
  if (hs == connections_.end()) {
    return nullptr;
  }

  auto devices = hs->second;
  for (client_t* connected_device : devices) {
    auto uinf = connected_device->GetServerHostInfo();
    if (uinf.MakeUserRpc() == user) {
      return connected_device;
    }
  }
  return nullptr;
}

}  // namespace server
}  // namespace fastotv
