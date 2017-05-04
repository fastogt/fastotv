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

#include <string>
#include <vector>

#include "network/tcp/tcp_server.h"

#include <common/logger.h>
#include <common/threads/types.h>

#include "network/tcp/tcp_client.h"

namespace {

#ifdef OS_WIN
struct WinsockInit {
  WinsockInit() {
    WSADATA d;
    int res = WSAStartup(MAKEWORD(2, 2), &d);
    if (res != 0) {
      DEBUG_MSG_PERROR("WSAStartup", res);
      exit(EXIT_FAILURE);
    }
  }
  ~WinsockInit() { WSACleanup(); }
} winsock_init;
#endif
struct SigIgnInit {
  SigIgnInit() {
#if defined(COMPILER_MINGW)
#elif defined(COMPILER_MSVC)
#else
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
      DEBUG_MSG_PERROR("signal", errno);
      exit(EXIT_FAILURE);
    }
#endif
  }
} sig_init;

}  // namespace

namespace fasto {
namespace fastotv {
namespace network {
namespace tcp {

// server
TcpServer::TcpServer(const common::net::HostAndPort& host, IoLoopObserver* observer)
    : IoLoop(observer), sock_(host), accept_io_((struct ev_io*)calloc(1, sizeof(struct ev_io))) {
  accept_io_->data = this;
}

TcpServer::~TcpServer() {
  free(accept_io_);
  accept_io_ = NULL;
}

IoLoop* TcpServer::FindExistServerByHost(const common::net::HostAndPort& host) {
  if (!host.IsValid()) {
    return nullptr;
  }

  auto find_by_host = [host](IoLoop* loop) -> bool {
    TcpServer* server = static_cast<TcpServer*>(loop);
    if (!server) {
      return false;
    }

    return server->GetHost() == host;
  };

  return FindExistLoopByPredicate(find_by_host);
}

TcpClient* TcpServer::CreateClient(const common::net::socket_info& info) {
  return new TcpClient(this, info);
}

void TcpServer::PreLooped(LibEvLoop* loop) {
  int fd = sock_.fd();
  ev_io_init(accept_io_, accept_cb, fd, EV_READ);
  loop->StartIO(accept_io_);
  IoLoop::PreLooped(loop);
}

void TcpServer::PostLooped(LibEvLoop* loop) {
  IoLoop::PostLooped(loop);
}

void TcpServer::Stoped(LibEvLoop* loop) {
  common::Error err = sock_.close();
  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
  }

  loop->StopIO(accept_io_);
  IoLoop::Stoped(loop);
}

common::Error TcpServer::Bind() {
  return sock_.bind();
}

common::Error TcpServer::Listen(int backlog) {
  return sock_.listen(backlog);
}

const char* TcpServer::ClassName() const {
  return "TcpServer";
}

common::net::HostAndPort TcpServer::GetHost() const {
  return sock_.host();
}

common::Error TcpServer::Accept(common::net::socket_info* info) {
  return sock_.accept(info);
}

void TcpServer::accept_cb(struct ev_loop* loop, struct ev_io* watcher, int revents) {
  if (!watcher || !watcher->data) {
    NOTREACHED();
    return;
  }

  TcpServer* pserver = reinterpret_cast<TcpServer*>(watcher->data);
  LibEvLoop* evloop = reinterpret_cast<LibEvLoop*>(ev_userdata(loop));
  CHECK(pserver && pserver->loop_ == evloop);

  if (EV_ERROR & revents) {
    DNOTREACHED();
    return;
  }

  CHECK(watcher->fd == pserver->sock_.fd());

  common::net::socket_info sinfo;
  common::Error err = pserver->Accept(&sinfo);

  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
    return;
  }

  IoClient* pclient = pserver->CreateClient(sinfo);
  pserver->RegisterClient(pclient);
}

}  // namespace tcp
}
}  // namespace fastotv
}  // namespace fasto
