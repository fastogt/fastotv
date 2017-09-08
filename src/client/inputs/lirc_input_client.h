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

#include <functional>  // for function
#include <string>      // for string

#include <common/error.h>            // for Error
#include <common/libev/io_client.h>  // for IoClient
#include <common/macros.h>           // for WARN_UNUSED_RESULT
#include <common/net/socket_tcp.h>   // for SocketHolder

struct lirc_config;

namespace common {
namespace libev {
class IoLoop;
}
}  // namespace common

namespace fastotv {
namespace client {
namespace inputs {

common::Error LircInit(int* fd, struct lirc_config** cfg) WARN_UNUSED_RESULT;
common::Error LircDeinit(int fd, struct lirc_config** cfg) WARN_UNUSED_RESULT;

class LircInputClient : public common::libev::IoClient {
 public:
  typedef std::function<void(const std::string&)> read_callback_t;
  LircInputClient(common::libev::IoLoop* server, int fd, struct lirc_config* cfg);

  virtual int GetFd() const override;

  common::Error ReadWithCallback(read_callback_t cb) WARN_UNUSED_RESULT;

 private:
  virtual common::Error CloseImpl() override;

  virtual common::Error Write(const char* data, size_t size, size_t* nwrite) override final WARN_UNUSED_RESULT;
  virtual common::Error Read(char* out, size_t max_size, size_t* nread) override final WARN_UNUSED_RESULT;

  common::net::SocketHolder sock_;
  struct lirc_config* cfg_;
};
}  // namespace inputs
}  // namespace client
}  // namespace fastotv
