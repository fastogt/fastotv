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

#include "client/ioservice.h"

#include <string>

#include <common/convert2string.h>
#include <common/file_system.h>
#include <common/logger.h>
#include <common/application/application.h>
#include <common/threads/thread_manager.h>

#include "client/inner/inner_tcp_handler.h"
#include "client/inner/inner_tcp_server.h"

#ifdef HAVE_LIRC
#include "client/inputs/lirc_input_client.h"
#include "client/core/events/lirc_events.h"
#endif

#include "server_config.h"

namespace fasto {
namespace fastotv {
namespace client {

namespace {
class PrivateHandler : public inner::InnerTcpHandler {
 public:
  typedef inner::InnerTcpHandler base_class;
  PrivateHandler(const common::net::HostAndPort& innerHost, AuthInfoSPtr ainf)
      : base_class(innerHost, ainf)
#ifdef HAVE_LIRC
        ,
        client_(nullptr)
#endif
  {
  }

  virtual void PreLooped(common::libev::IoLoop* server) override {
#ifdef HAVE_LIRC
    int fd;
    struct lirc_config* lcd = NULL;
    common::Error err = inputs::LircInit(&fd, &lcd);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    } else {
      client_ = new inputs::LircInputClient(server, fd, lcd);
      server->RegisterClient(client_);
    }
#endif
    base_class::PreLooped(server);
  }

  virtual void Closed(common::libev::IoClient* client) override {
#ifdef HAVE_LIRC
    if (client == client_) {
      client_ = nullptr;
      return;
    }
#endif
    base_class::Closed(client);
  }

  virtual void DataReceived(common::libev::IoClient* client) override {
#ifdef HAVE_LIRC
    if (client == client_) {
      auto cb = [this](const std::string& code) {
        LircCode lcode;
        if (!common::ConvertFromString(code, &lcode)) {
          WARNING_LOG() << "Unknown lirc code: " << code;
          return;
        }

        core::events::LircPressInfo linf;
        linf.code = lcode;
        core::events::LircPressEvent* levent = new core::events::LircPressEvent(this, linf);
        fApp->PostEvent(levent);
      };
      client_->ReadWithCallback(cb);
      return;
    }
#endif
    base_class::DataReceived(client);
  }

  virtual void PostLooped(common::libev::IoLoop* server) override {
    UNUSED(server);
#ifdef HAVE_LIRC
    if (client_) {
      inputs::LircInputClient* connection = client_;
      connection->Close();
      delete connection;
    }
#endif
    base_class::PostLooped(server);
  }
#ifdef HAVE_LIRC
  inputs::LircInputClient* client_;
#endif
};
}

IoService::IoService()
    : ILoopController(), loop_thread_(THREAD_MANAGER()->CreateThread(&IoService::Exec, this)) {
}

void IoService::Start() {
  ILoopController::Start();
}

void IoService::Stop() {
  ILoopController::Stop();
}

IoService::~IoService() {
}

void IoService::ConnectToServer() const {
  PrivateHandler* handler = static_cast<PrivateHandler*>(handler_);
  client::inner::InnerTcpServer* server = static_cast<client::inner::InnerTcpServer*>(loop_);
  if (handler) {
    auto cb = [handler, server]() { handler->Connect(server); };
    ExecInLoopThread(cb);
  }
}

void IoService::DisconnectFromServer() const {
  PrivateHandler* handler = static_cast<PrivateHandler*>(handler_);
  if (handler) {
    auto cb = [handler]() { handler->DisConnect(common::Error()); };
    ExecInLoopThread(cb);
  }
}

void IoService::RequestChannels() const {
  PrivateHandler* handler = static_cast<PrivateHandler*>(handler_);
  if (handler) {
    auto cb = [handler]() { handler->RequestChannels(); };
    ExecInLoopThread(cb);
  }
}

common::libev::IoLoopObserver* IoService::CreateHandler() {
  PrivateHandler* handler = new PrivateHandler(
      g_service_host, common::make_shared<AuthInfo>(USER_LOGIN, USER_PASSWORD, 0));
  return handler;
}

common::libev::IoLoop* IoService::CreateServer(common::libev::IoLoopObserver* handler) {
  client::inner::InnerTcpServer* serv = new client::inner::InnerTcpServer(handler);
  serv->SetName("local_inner_server");
  return serv;
}

void IoService::HandleStarted() {
  bool result = loop_thread_->Start();
  DCHECK(result);
}

void IoService::HandleStoped() {
  loop_thread_->JoinAndGet();
}

}  // namespace network
}  // namespace fastotv
}  // namespace fasto
