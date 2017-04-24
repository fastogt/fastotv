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

#include <string>

#include <common/patterns/crtp_pattern.h>
#include <common/net/socket_tcp.h>

#include "network/io_client.h"

namespace fasto {
namespace fastotv {
namespace network {
namespace tcp {

class TcpClient : public IoClient {
 public:
  TcpClient(IoLoop* server, const common::net::socket_info& info, flags_t flags = EV_READ);
  virtual ~TcpClient();

  common::net::socket_info Info() const;

 protected:
  virtual int Fd() const override;

  virtual common::Error Write(const char* data, size_t size, size_t* nwrite) override;
  virtual common::Error Read(char* out, size_t size, size_t* nread) override;
  virtual void CloseImpl() override;

 private:
  common::net::SocketHolder sock_;
};

}  // namespace tcp
}
}  // namespace fastotv
}  // namespace fasto
