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

ServerHandlerHost::ServerHandlerHost() : tcp::ITcpLoopObserver() {}

void ServerHandlerHost::preLooped(tcp::ITcpLoop* server) {}

void ServerHandlerHost::accepted(tcp::TcpClient* client) {}

void ServerHandlerHost::moved(tcp::TcpClient* client) {}

void ServerHandlerHost::closed(tcp::TcpClient* client) {}

void ServerHandlerHost::timerEmited(tcp::ITcpLoop* server, timer_id_t id) {}

void ServerHandlerHost::dataReceived(tcp::TcpClient* client) {}

void ServerHandlerHost::dataReadyToWrite(tcp::TcpClient* client) {}

void ServerHandlerHost::postLooped(tcp::ITcpLoop* server) {}

ServerHandlerHost::~ServerHandlerHost() {}

ServerHost::ServerHost(const common::net::HostAndPort& host)
    : stop_(false), server_(nullptr), handler_(nullptr) {
  handler_ = new ServerHandlerHost;
  server_ = new tcp::TcpServer(host);
}

ServerHost::~ServerHost() {
  destroy(&server_);
  destroy(&handler_);
}

void ServerHost::stop() {
  stop_ = true;
  server_->stop();
}

int ServerHost::exec() {
  std::shared_ptr<common::threads::Thread<int> > connection_thread =
      THREAD_MANAGER()->CreateThread(&exec_server, server_);
  while (!stop_) {
  }

  connection_thread->JoinAndGet();
  return EXIT_SUCCESS;
}

void ServerHost::setConfig(const Config& conf) {
  rstorage_.setConfig(conf.server.redis);
}

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
