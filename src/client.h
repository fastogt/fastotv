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

#pragma once

#include <common/libev/tcp/tcp_client.h>  // for TcpClient

#include "protocol/protocol.h"

namespace fastotv {

class Client : public common::libev::tcp::TcpClient {
 public:
  Client(common::libev::IoLoop* server, const common::net::socket_info& info);
  ~Client() override;

  const char* ClassName() const override;
};

class ProtocoledClient : public protocol::ProtocolClient<Client> {
 public:
  typedef protocol::ProtocolClient<Client> base_class;
  ProtocoledClient(common::libev::IoLoop* server, const common::net::socket_info& info);
};

}  // namespace fastotv
