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

#include "server/server_host.h"

#include <unistd.h>

#include <string>

#include <common/threads/thread_manager.h>
#include <common/logger.h>

#include "commands/commands.h"

#include "network/tcp/tcp_client.h"

#include "inner/inner_tcp_client.h"
#include "inner/inner_server_command_seq_parser.h"

#include "server/inner/inner_tcp_server.h"
#include "server/inner/inner_tcp_handler.h"

#define BUF_SIZE 4096
#define UNKNOWN_CLIENT_NAME "Unknown"

namespace fasto {
namespace fastotv {
namespace {
int exec_server(network::tcp::TcpServer* server) {
  common::Error err = server->Bind();
  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
    return EXIT_FAILURE;
  }

  err = server->Listen(5);
  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
    return EXIT_FAILURE;
  }

  return server->Exec();
}
}
namespace server {

ServerHost::ServerHost(const common::net::HostAndPort& host)
    : stop_(false), handler_(nullptr), server_(nullptr) {
  handler_ = new inner::InnerTcpHandlerHost(this);
  server_ = new inner::InnerTcpServer(host, handler_);
  server_->SetName("inner_server");
}

ServerHost::~ServerHost() {
  destroy(&server_);
  destroy(&handler_);
}

void ServerHost::stop() {
  std::unique_lock<std::mutex> lock(stop_mutex_);
  stop_ = true;
  server_->Stop();
  stop_cond_.notify_all();
}

int ServerHost::exec() {
  common::shared_ptr<common::threads::Thread<int> > connection_thread =
      THREAD_MANAGER()->CreateThread(&exec_server, server_);
  bool result = connection_thread->Start();
  if (!result) {
    NOTREACHED();
    return EXIT_FAILURE;
  }

  while (!stop_) {
    common::unique_lock<common::mutex> lock(stop_mutex_);
    std::cv_status interrupt_status =
        stop_cond_.wait_for(lock, std::chrono::seconds(timeout_seconds));
    if (interrupt_status == std::cv_status::no_timeout) {  // if notify
      if (stop_) {
        break;
      }
    } else {  // timeout
    }
  }

  return connection_thread->JoinAndGet();
}

bool ServerHost::UnRegisterInnerConnectionByHost(network::tcp::TcpClient* connection) {
  inner::InnerTcpClient* iconnection = static_cast<inner::InnerTcpClient*>(connection);
  if (!iconnection) {
    DNOTREACHED();
    return false;
  }

  AuthInfo hinf = iconnection->ServerHostInfo();
  if (!hinf.IsValid()) {
    return false;
  }

  std::string login = hinf.login;
  connections_.erase(login);
  return true;
}

bool ServerHost::RegisterInnerConnectionByUser(const AuthInfo& user, network::tcp::TcpClient* connection) {
  CHECK(user.IsValid());
  inner::InnerTcpClient* iconnection = static_cast<inner::InnerTcpClient*>(connection);
  if (!iconnection) {
    DNOTREACHED();
    return false;
  }

  iconnection->SetServerHostInfo(user);

  std::string login = user.login;
  connections_[login] = iconnection;
  connection->SetName(login);
  return true;
}

common::Error ServerHost::FindUserAuth(const AuthInfo& user) const {
  return rstorage_.FindUserAuth(user);
}

common::Error ServerHost::FindUser(const AuthInfo& auth, UserInfo* uinf) const {
  return rstorage_.FindUser(auth, uinf);
}

inner::InnerTcpClient* ServerHost::FindInnerConnectionByLogin(const std::string& login) const {
  inner_connections_type::const_iterator hs = connections_.find(login);
  if (hs == connections_.end()) {
    return nullptr;
  }

  return (*hs).second;
}

void ServerHost::SetConfig(const Config& conf) {
  rstorage_.SetConfig(conf.server.redis);
  handler_->SetStorageConfig(conf.server.redis);
}

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
