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

#include <stdint.h>  // for uint32_t

#include <string>  // for string

#include <common/error.h>                 // for Error
#include <common/libev/tcp/tcp_client.h>  // for TcpClient
#include <common/macros.h>                // for WARN_UNUSED_RESULT

#include "commands/commands.h"  // for cmd_approve_t, cmd_request_t

namespace common {
namespace libev {
class IoLoop;
}
}  // namespace common
namespace common {
namespace net {
class socket_info;
}
}  // namespace common

namespace fasto {
namespace fastotv {
namespace inner {

class InnerClient : public common::libev::tcp::TcpClient {
 public:
  typedef uint32_t protocoled_size_t;  // sizeof 4 byte
  InnerClient(common::libev::IoLoop* server, const common::net::socket_info& info);
  const char* ClassName() const override;

  common::Error Write(const cmd_request_t& request) WARN_UNUSED_RESULT;
  common::Error Write(const cmd_responce_t& responce) WARN_UNUSED_RESULT;
  common::Error Write(const cmd_approve_t& approve) WARN_UNUSED_RESULT;

  common::Error ReadDataSize(protocoled_size_t* sz) WARN_UNUSED_RESULT;
  common::Error ReadMessage(char* out, protocoled_size_t size) WARN_UNUSED_RESULT;
  common::Error ReadCommand(std::string* out) WARN_UNUSED_RESULT;

 private:
  common::Error WriteInner(const std::string& data) WARN_UNUSED_RESULT;
  using common::libev::tcp::TcpClient::Write;
  using common::libev::tcp::TcpClient::Read;
};

}  // namespace inner
}  // namespace fastotv
}  // namespace fasto
