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

#include "server/server_host.h"

#include <unistd.h>

#include <string>

#include <common/threads/thread_manager.h>
#include <common/logger.h>

#include "commands/commands.h"

#include <common/libev/tcp/tcp_client.h>

#include "inner/inner_tcp_client.h"
#include "inner/inner_server_command_seq_parser.h"

#include "server/inner/inner_tcp_server.h"
#include "server/inner/inner_tcp_handler.h"

#define BUF_SIZE 4096
#define UNKNOWN_CLIENT_NAME "Unknown"

namespace fasto {
namespace fastotv {
namespace {
int exec_server(common::libev::tcp::TcpServer* server) {
  common::Error err = server->Bind();
  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
    return EXIT_FAILURE;
  }

  err = server->Listen(5);
  if (err && err->IsError()) {
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

common::Error ServerHost::UnRegisterInnerConnectionByHost(common::libev::IoClient* connection) {
  inner::InnerTcpClient* iconnection = static_cast<inner::InnerTcpClient*>(connection);
  if (!iconnection) {
    DNOTREACHED();
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  user_id_t uid = iconnection->GetUid();
  if (uid.empty()) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  connections_.erase(uid);
  return common::Error();
}

common::Error ServerHost::RegisterInnerConnectionByUser(user_id_t user_id,
                                                        const AuthInfo& user,
                                                        common::libev::IoClient* connection) {
  CHECK(user.IsValid());
  inner::InnerTcpClient* iconnection = static_cast<inner::InnerTcpClient*>(connection);
  if (!iconnection) {
    DNOTREACHED();
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  iconnection->SetServerHostInfo(user);
  iconnection->SetUid(user_id);

  std::string login = user.login;
  connections_[user_id] = iconnection;
  connection->SetName(login);
  return common::Error();
}

common::Error ServerHost::FindUserAuth(const AuthInfo& user, user_id_t* uid) const {
  return rstorage_.FindUserAuth(user, uid);
}

common::Error ServerHost::FindUser(const AuthInfo& auth, user_id_t* uid, UserInfo* uinf) const {
  return rstorage_.FindUser(auth, uid, uinf);
}

inner::InnerTcpClient* ServerHost::FindInnerConnectionByID(user_id_t user_id) const {
  inner_connections_type::const_iterator hs = connections_.find(user_id);
  if (hs == connections_.end()) {
    return nullptr;
  }

  return (*hs).second;
}

void ServerHost::SetConfig(const Config& conf) {
  rstorage_.SetConfig(conf.server.redis);
  handler_->SetConfig(conf);
}

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
