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

#ifdef HAVE_LIRC
#include "client/core/inputs/lirc_input_client.h"
#endif

#include <common/application/application.h>

#include "server_config.h"

namespace fasto {
namespace fastotv {
namespace client {

namespace {
class PrivateHandler : public inner::InnerTcpHandler {
 public:
  typedef inner::InnerTcpHandler base_class;
  PrivateHandler(const common::net::HostAndPort& innerHost, const AuthInfo& ainf)
      : base_class(innerHost, ainf)
#ifdef HAVE_LIRC
        ,
        client_(nullptr)
#endif
  {
  }

  virtual void PreLooped(network::IoLoop* server) override {
#ifdef HAVE_LIRC
    int fd;
    common::Error err = core::inputs::LircInit(&fd);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    } else {
      client_ = new core::inputs::LircInputClient(server, fd);
      server->RegisterClient(client_);
    }
#endif
    base_class::PreLooped(server);
  }

  void Closed(network::IoClient* client) {
#ifdef HAVE_LIRC
    if (client == client_) {
      client_ = nullptr;
      return;
    }
#endif
    base_class::Closed(client);
  }

  void DataReceived(network::IoClient* client) {
#ifdef HAVE_LIRC
    if (client == client_) {
      return;
    }
#endif
    base_class::DataReceived(client);
  }

  virtual void PostLooped(network::IoLoop* server) override {
    UNUSED(server);
#ifdef HAVE_LIRC
    if (client_) {
      core::inputs::LircInputClient* connection = client_;
      connection->Close();
      delete connection;
    }
#endif
    base_class::PostLooped(server);
  }
#ifdef HAVE_LIRC
  core::inputs::LircInputClient* client_;
#endif
};
}

NetworkController::NetworkController() : ILoopThreadController() {
}

void NetworkController::Start() {
  ILoopThreadController::Start();
}

void NetworkController::Stop() {
  ILoopThreadController::Stop();
}

NetworkController::~NetworkController() {
}

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

network::IoLoopObserver* NetworkController::CreateHandler() {
  client::inner::InnerTcpHandler* handler = new PrivateHandler(g_service_host, GetAuthInfo());
  return handler;
}

network::IoLoop* NetworkController::CreateServer(network::IoLoopObserver* handler) {
  client::inner::InnerTcpServer* serv = new client::inner::InnerTcpServer(handler);
  serv->SetName("local_inner_server");
  return serv;
}

}  // namespace network
}  // namespace fastotv
}  // namespace fasto
