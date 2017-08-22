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

#include <stdlib.h>  // for EXIT_FAILURE

#include <condition_variable>  // for cv_status, cv_status::no...
#include <mutex>               // for mutex, unique_lock
#include <string>              // for string

#include <common/libev/tcp/tcp_server.h>    // for TcpServer
#include <common/logger.h>                  // for COMPACT_LOG_FILE_CRIT
#include <common/threads/thread_manager.h>  // for THREAD_MANAGER

#include "inner/inner_tcp_client.h"  // for InnerTcpClient

#include "server/inner/inner_tcp_handler.h"  // for InnerTcpHandlerHost
#include "server/inner/inner_tcp_server.h"

#define BUF_SIZE 4096
#define UNKNOWN_CLIENT_NAME "Unknown"

namespace fasto {
namespace fastotv {
namespace {
int exec_server(common::libev::tcp::TcpServer* server) {
  common::Error err = server->Bind(true);
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
}  // namespace
namespace server {

ServerHost::ServerHost(const Config& config)
    : stop_(false), handler_(nullptr), server_(nullptr), rstorage_(), config_(config) {
  handler_ = new inner::InnerTcpHandlerHost(this, config);
  server_ = new inner::InnerTcpServer(config.server.host, handler_);
  server_->SetName("inner_server");

  rstorage_.SetConfig(config.server.redis);
}

ServerHost::~ServerHost() {
  destroy(&server_);
  destroy(&handler_);
}

void ServerHost::Stop() {
  std::unique_lock<std::mutex> lock(stop_mutex_);
  stop_ = true;
  server_->Stop();
  stop_cond_.notify_all();
}

int ServerHost::Exec() {
  std::shared_ptr<common::threads::Thread<int> > connection_thread =
      THREAD_MANAGER()->CreateThread(&exec_server, server_);
  bool result = connection_thread->Start();
  if (!result) {
    NOTREACHED();
    return EXIT_FAILURE;
  }

  while (!stop_) {
    std::unique_lock<std::mutex> lock(stop_mutex_);
    std::cv_status interrupt_status = stop_cond_.wait_for(lock, std::chrono::seconds(timeout_seconds));
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
    return common::make_inval_error_value(common::ErrorValue::E_ERROR);
  }

  user_id_t uid = iconnection->GetUid();
  if (uid.empty()) {
    return common::make_inval_error_value(common::ErrorValue::E_ERROR);
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
    return common::make_inval_error_value(common::ErrorValue::E_ERROR);
  }

  iconnection->SetServerHostInfo(user);
  iconnection->SetUid(user_id);

  login_t login = user.GetLogin();
  connections_[user_id].push_back(iconnection);
  connection->SetName(login);
  return common::Error();
}

common::Error ServerHost::FindUserAuth(const AuthInfo& user, user_id_t* uid) const {
  return rstorage_.FindUserAuth(user, uid);
}

common::Error ServerHost::FindUser(const AuthInfo& auth, user_id_t* uid, UserInfo* uinf) const {
  return rstorage_.FindUser(auth, uid, uinf);
}

inner::InnerTcpClient* ServerHost::FindInnerConnectionByUserIDAndDeviceID(user_id_t user_id, device_id_t dev) const {
  inner_connections_type::const_iterator hs = connections_.find(user_id);
  if (hs == connections_.end()) {
    return nullptr;
  }

  auto devices = (*hs).second;
  for (inner::InnerTcpClient* connected_device : devices) {
    AuthInfo uinf = connected_device->GetServerHostInfo();
    if (uinf.GetDeviceID() == dev) {
      return connected_device;
    }
  }
  return nullptr;
}

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
