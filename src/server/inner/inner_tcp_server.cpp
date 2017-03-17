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

#include "server/inner/inner_tcp_server.h"

#include "server/inner/inner_tcp_client.h"

namespace fasto {
namespace fastotv {
namespace server {
namespace inner {

InnerTcpServer::InnerTcpServer(const common::net::HostAndPort& host,
                               network::tcp::ITcpLoopObserver* observer)
    : TcpServer(host, observer) {}

const char* InnerTcpServer::ClassName() const {
  return "InnerTcpServer";
}

network::tcp::TcpClient* InnerTcpServer::CreateClient(const common::net::socket_info& info) {
  InnerTcpClient* client = new InnerTcpClient(this, info);
  return client;
}

}  // namespace inner
}  // namespace server
}  // namespace fastotv
}  // namespace fasto
