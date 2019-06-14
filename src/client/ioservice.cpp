/*  Copyright (C) 2014-2019 FastoGT. All right reserved.

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

#include <string>  // for string

#include <common/application/application.h>  // for fApp
#include <common/error.h>                    // for DEBUG_MSG_ERROR, Error
#include <common/logger.h>                   // for COMPACT_LOG_FILE_CRIT
#include <common/macros.h>                   // for DCHECK, UNUSED
#include <common/threads/thread_manager.h>   // for THREAD_MANAGER

#include "client/inner/inner_tcp_handler.h"  // for InnerTcpHandler, StartC...
#include "client/inner/inner_tcp_server.h"   // for InnerTcpServer

#ifdef HAVE_LIRC
#include <player/gui/lirc_events.h>           // for ConvertFromString, Lirc...
#include "client/inputs/lirc_input_client.h"  // for LircInit, LircInputClient
#endif

namespace common {
namespace libev {
class IoClient;
}
}  // namespace common

namespace fastotv {
namespace client {

namespace {
class PrivateHandler : public inner::InnerTcpHandler {
 public:
  typedef inner::InnerTcpHandler base_class;
  explicit PrivateHandler(const inner::StartConfig& conf)
      : base_class(conf)
#ifdef HAVE_LIRC
        ,
        client_(nullptr)
#endif
  {
  }

  ~PrivateHandler() override {}

  void PreLooped(common::libev::IoLoop* server) override {
#ifdef HAVE_LIRC
    int fd = INVALID_DESCRIPTOR;
    struct lirc_config* lcd = nullptr;
    common::ErrnoError err = inputs::LircInit(&fd, &lcd);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    } else {
      client_ = new inputs::LircInputClient(server, fd, lcd);
      server->RegisterClient(client_);
    }
#endif
    base_class::PreLooped(server);
  }

  void Closed(common::libev::IoClient* client) override {
#ifdef HAVE_LIRC
    if (client == client_) {
      client_ = nullptr;
      return;
    }
#endif
    base_class::Closed(client);
  }

  void DataReceived(common::libev::IoClient* client) override {
#ifdef HAVE_LIRC
    if (client == client_) {
      auto cb = [this](const std::string& code) {
        LircCode lcode;
        if (!common::ConvertFromString(code, &lcode)) {
          WARNING_LOG() << "Unknown lirc code: " << code;
          return;
        }

        fastoplayer::gui::events::LircPressInfo linf;
        linf.code = lcode;
        fastoplayer::gui::events::LircPressEvent* levent = new fastoplayer::gui::events::LircPressEvent(this, linf);
        fApp->PostEvent(levent);
      };
      client_->ReadWithCallback(cb);
      return;
    }
#endif
    base_class::DataReceived(client);
  }

  void PostLooped(common::libev::IoLoop* server) override {
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
}  // namespace

IoService::IoService(const commands_info::AuthInfo& ainf)
    : ILoopController(), ainf_(ainf), loop_thread_(THREAD_MANAGER()->CreateThread(&IoService::Exec, this)) {}

bool IoService::IsRunning() const {
  return loop_->IsRunning();
}

void IoService::Start() {
  ILoopController::Start();
}

void IoService::Stop() {
  ILoopController::Stop();
}

IoService::~IoService() {}

void IoService::ConnectToServer() const {
  PrivateHandler* handler = static_cast<PrivateHandler*>(handler_);
  client::inner::InnerTcpServer* server = static_cast<client::inner::InnerTcpServer*>(loop_);
  if (handler) {
    auto cb = [handler, server]() { handler->Connect(server); };
    ExecInLoopThread(cb);
  }
}

void IoService::ActivateRequest() const {
  PrivateHandler* handler = static_cast<PrivateHandler*>(handler_);
  if (handler) {
    auto cb = [handler]() { handler->ActivateRequest(); };
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

void IoService::RequestServerInfo() const {
  PrivateHandler* handler = static_cast<PrivateHandler*>(handler_);
  if (handler) {
    auto cb = [handler]() { handler->RequestServerInfo(); };
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

void IoService::RequesRuntimeChannelInfo(stream_id sid) const {
  PrivateHandler* handler = static_cast<PrivateHandler*>(handler_);
  if (handler) {
    auto cb = [handler, sid]() { handler->RequesRuntimeChannelInfo(sid); };
    ExecInLoopThread(cb);
  }
}

common::libev::IoLoopObserver* IoService::CreateHandler() {
  inner::StartConfig conf;
  conf.inner_host = common::net::HostAndPort(PROJECT_SERVER_HOST, PROJECT_SERVER_PORT);
  conf.ainf = ainf_;
  PrivateHandler* handler = new PrivateHandler(conf);
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

void IoService::HandleStopped() {
  loop_thread_->JoinAndGet();
}

}  // namespace client
}  // namespace fastotv
