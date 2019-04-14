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

#pragma once

#include <common/libev/tcp/tcp_client.h>  // for TcpClient

#include "protocol/protocol.h"

namespace fastotv {
namespace inner {

class InnerClient : public common::libev::tcp::TcpClient {
 public:
  InnerClient(common::libev::IoLoop* server, const common::net::socket_info& info);
  virtual ~InnerClient();

  const char* ClassName() const override;
};

class ProtocoledInnerClient : public protocol::ProtocolClient<InnerClient> {
 public:
  typedef protocol::ProtocolClient<InnerClient> base_class;
  ProtocoledInnerClient(common::libev::IoLoop* server, const common::net::socket_info& info);
};

}  // namespace inner
}  // namespace fastotv
