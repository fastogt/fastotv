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

#include <string>

#include <common/libev/tcp/tcp_client.h>  // for TcpClient

#include "commands/commands.h"

namespace common {
class IEDcoder;
}

namespace fastotv {
namespace inner {

class InnerClient : public common::libev::tcp::TcpClient {
 public:
  typedef uint32_t protocoled_size_t;  // sizeof 4 byte
  enum { MAX_COMMAND_SIZE = 1024 * 8 };
  InnerClient(common::libev::IoLoop* server, const common::net::socket_info& info);
  virtual ~InnerClient();

  const char* ClassName() const override;

  common::ErrnoError Write(const common::protocols::three_way_handshake::cmd_request_t& request) WARN_UNUSED_RESULT;
  common::ErrnoError Write(const common::protocols::three_way_handshake::cmd_response_t& responce) WARN_UNUSED_RESULT;
  common::ErrnoError Write(const common::protocols::three_way_handshake::cmd_approve_t& approve) WARN_UNUSED_RESULT;

  common::ErrnoError ReadCommand(std::string* out) WARN_UNUSED_RESULT;

 private:
  common::ErrnoError ReadDataSize(protocoled_size_t* sz) WARN_UNUSED_RESULT;
  common::ErrnoError ReadMessage(char* out, protocoled_size_t size) WARN_UNUSED_RESULT;

  common::ErrnoError WriteMessage(const std::string& message) WARN_UNUSED_RESULT;
  using common::libev::tcp::TcpClient::Read;
  using common::libev::tcp::TcpClient::Write;

 private:
  common::IEDcoder* compressor_;
};

}  // namespace inner
}  // namespace fastotv
