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

#include <common/threads/thread.h>
#include <common/threads/types.h>

#include "inner/inner_server_command_seq_parser.h"

#include "network/tcp/tcp_server.h"

namespace fasto {
namespace fastotv {
namespace server {
namespace inner {

class InnerTcpServer : public network::tcp::TcpServer {
 public:
  InnerTcpServer(const common::net::HostAndPort& host, network::IoLoopObserver* observer);
  virtual const char* ClassName() const override;

 private:
  virtual network::tcp::TcpClient* CreateClient(const common::net::socket_info& info) override;
};

}  // namespace inner
}  // namespace server
}  // namespace fastotv
}  // namespace fasto
