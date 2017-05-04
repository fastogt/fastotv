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

#pragma once

#include "network/io_server.h"

#include "network/tcp/tcp_client.h"

#define INVALID_TIMER_ID -1

namespace fasto {
namespace fastotv {
namespace network {
namespace tcp {

class TcpServer : public network::IoLoop {
 public:
  explicit TcpServer(const common::net::HostAndPort& host, IoLoopObserver* observer = nullptr);
  virtual ~TcpServer();

  common::Error Bind() WARN_UNUSED_RESULT;
  common::Error Listen(int backlog) WARN_UNUSED_RESULT;

  const char* ClassName() const override;
  common::net::HostAndPort GetHost() const;

  static network::IoLoop* FindExistServerByHost(const common::net::HostAndPort& host);

 private:
  virtual TcpClient* CreateClient(const common::net::socket_info& info) override;
  virtual void PreLooped(LibEvLoop* loop) override;
  virtual void PostLooped(LibEvLoop* loop) override;

  virtual void Stoped(LibEvLoop* loop) override;

  static void accept_cb(struct ev_loop* loop, struct ev_io* watcher, int revents);

  common::Error Accept(common::net::socket_info* info) WARN_UNUSED_RESULT;

  common::net::ServerSocketTcp sock_;
  ev_io* accept_io_;
};

}  // namespace tcp
}
}  // namespace fastotv
}  // namespace fasto
