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

#include "network_controller.h"

#include <string>

#include <common/convert2string.h>
#include <common/file_system.h>
#include <common/logger.h>

#include "client/inner/inner_tcp_handler.h"
#include "client/inner/inner_tcp_server.h"

#include <common/application/application.h>

#include "server_config.h"

namespace fasto {
namespace fastotv {
namespace client {

NetworkController::NetworkController() : ILoopThreadController() {}

void NetworkController::Start() {
  ILoopThreadController::Start();
}

void NetworkController::Stop() {
  ILoopThreadController::Stop();
}

NetworkController::~NetworkController() {}

AuthInfo NetworkController::GetAuthInfo() {
  return AuthInfo(USER_LOGIN, USER_PASSWORD);
}

void NetworkController::RequestChannels() const {
  client::inner::InnerTcpHandler* handler = static_cast<client::inner::InnerTcpHandler*>(handler_);
  if (handler) {
    auto cb = [handler]() { handler->RequestChannels(); };
    ExecInLoopThread(cb);
  }
}

network::tcp::ITcpLoopObserver* NetworkController::CreateHandler() {
  client::inner::InnerTcpHandler* handler =
      new client::inner::InnerTcpHandler(g_service_host, GetAuthInfo());
  return handler;
}

network::tcp::ITcpLoop* NetworkController::CreateServer(network::tcp::ITcpLoopObserver* handler) {
  client::inner::InnerTcpServer* serv = new client::inner::InnerTcpServer(handler);
  serv->SetName("local_inner_server");
  return serv;
}

}  // namespace network
}  // namespace fastotv
}  // namespace fasto
