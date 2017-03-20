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

#include "server/inner/inner_tcp_client.h"

#include <common/logger.h>

#include "server/inner/inner_tcp_server.h"
#include "server/commands.h"

#define BUF_SIZE 4096

namespace fasto {
namespace fastotv {
namespace server {
namespace inner {

InnerTcpClient::InnerTcpClient(network::tcp::TcpServer* server,
                               const common::net::socket_info& info)
    : InnerClient(server, info), hinfo_(), uid_() {}

const char* InnerTcpClient::ClassName() const {
  return "InnerTcpClient";
}

InnerTcpClient::~InnerTcpClient() {}

void InnerTcpClient::SetServerHostInfo(const AuthInfo& info) {
  hinfo_ = info;
}

AuthInfo InnerTcpClient::ServerHostInfo() const {
  return hinfo_;
}

void InnerTcpClient::SetUid(user_id_t id) {
  uid_ = id;
}

user_id_t InnerTcpClient::GetUid() const {
  return uid_;
}

}  // namespace inner
}  // namespace server
}  // namespace fastotv
}  // namespace fasto
