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

#define BUF_SIZE 4096

namespace fasto {
namespace fastotv {
namespace server {

namespace {
int exec_server(tcp::TcpServer* server) {
  common::Error err = server->bind();
  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
    return EXIT_FAILURE;
  }

  err = server->listen(5);
  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
    return EXIT_FAILURE;
  }

  return server->exec();
}
}

ServerHost::ServerHost(const common::net::HostAndPort& host)
    : stop_(false), server_(nullptr), handler_(nullptr) {
  handler_ = new inner::InnerServerHandlerHost(this);
  server_ = new inner::InnerTcpServer(host, handler_);
  server_->setName("inner_server");
}

ServerHost::~ServerHost() {
  destroy(&server_);
  destroy(&handler_);
}

void ServerHost::stop() {
  std::unique_lock<std::mutex> lock(stop_mutex_);
  stop_ = true;
  server_->stop();
  stop_cond_.notify_all();
}

int ServerHost::exec() {
  std::shared_ptr<common::threads::Thread<int> > connection_thread =
      THREAD_MANAGER()->CreateThread(&exec_server, server_);
  while (!stop_) {
    std::unique_lock<std::mutex> lock(stop_mutex_);
    std::cv_status interrupt_status =
        stop_cond_.wait_for(lock, std::chrono::seconds(timeout_seconds));
    if (interrupt_status == std::cv_status::no_timeout) {  // if notify
      if (stop_) {
        break;
      }
    } else {
    }
  }

  connection_thread->JoinAndGet();
  return EXIT_SUCCESS;
}

bool ServerHost::unRegisterInnerConnectionByHost(tcp::TcpClient* connection) {
  inner::InnerTcpServerClient* iconnection = dynamic_cast<inner::InnerTcpServerClient*>(connection);
  if (!iconnection) {
    return false;
  }

  UserAuthInfo hinf = iconnection->serverHostInfo();
  if (!hinf.isValid()) {
    return false;
  }

  std::string login = hinf.login;
  connections_.erase(login);
  return true;
}

bool ServerHost::registerInnerConnectionByUser(const UserAuthInfo& user,
                                               tcp::TcpClient* connection) {
  CHECK(user.isValid());
  inner::InnerTcpServerClient* iconnection = dynamic_cast<inner::InnerTcpServerClient*>(connection);
  if (!iconnection) {
    return false;
  }

  iconnection->setServerHostInfo(user);

  std::string login = user.login;
  connections_[login] = iconnection;
  return true;
}

bool ServerHost::findUser(const UserAuthInfo& user) const {
  return rstorage_.findUser(user);
}

inner::InnerTcpServerClient* ServerHost::findInnerConnectionByLogin(const std::string& login) const {
  inner_connections_type::const_iterator hs = connections_.find(login);
  if (hs == connections_.end()) {
    return nullptr;
  }

  return (*hs).second;
}

void ServerHost::setConfig(const Config& conf) {
  rstorage_.setConfig(conf.server.redis);
}

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
