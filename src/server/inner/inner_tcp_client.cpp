/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

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

#include <common/libev/tcp/tcp_server.h>

namespace fastotv {
namespace server {
namespace inner {

const AuthInfo InnerTcpClient::anonim_user(USER_LOGIN, USER_PASSWORD, USER_DEVICE_ID);

InnerTcpClient::InnerTcpClient(common::libev::tcp::TcpServer* server, const common::net::socket_info& info)
    : base_class(server, info), hinfo_(), uid_(), current_stream_id_(invalid_stream_id) {}

bool InnerTcpClient::IsAnonimUser() const {
  return anonim_user == hinfo_;
}

const char* InnerTcpClient::ClassName() const {
  return "InnerTcpClient";
}

InnerTcpClient::~InnerTcpClient() {}

void InnerTcpClient::SetServerHostInfo(const AuthInfo& info) {
  hinfo_ = info;
}

AuthInfo InnerTcpClient::GetServerHostInfo() const {
  return hinfo_;
}

void InnerTcpClient::SetUid(user_id_t id) {
  uid_ = id;
}

user_id_t InnerTcpClient::GetUid() const {
  return uid_;
}

void InnerTcpClient::SetCurrentStreamId(stream_id sid) {
  current_stream_id_ = sid;
}

user_id_t InnerTcpClient::GetCurrentStreamId() const {
  return current_stream_id_;
}

}  // namespace inner
}  // namespace server
}  // namespace fastotv
