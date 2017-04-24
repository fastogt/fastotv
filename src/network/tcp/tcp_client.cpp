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

#include "network/tcp/tcp_client.h"

#include <inttypes.h>

#include <string>

#include <common/logger.h>

#include "network/tcp/tcp_server.h"

namespace fasto {
namespace fastotv {
namespace network {
namespace tcp {

TcpClient::TcpClient(IoLoop* server, const common::net::socket_info& info, flags_t flags)
    : network::IoClient(server, flags), sock_(info) {
}

TcpClient::~TcpClient() {
}

common::net::socket_info TcpClient::Info() const {
  return sock_.info();
}

int TcpClient::Fd() const {
  return sock_.fd();
}

common::Error TcpClient::Write(const char* data, size_t size, size_t* nwrite) {
  if (!data || !size || !nwrite) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  size_t total = 0;          // how many bytes we've sent
  size_t bytes_left = size;  // how many we have left to send

  while (total < size) {
    size_t n;
    common::Error err = sock_.write(data, size, &n);
    if (err && err->IsError()) {
      return err;
    }
    total += n;
    bytes_left -= n;
  }

  *nwrite = total;  // return number actually sent here
  return common::Error();
}

common::Error TcpClient::Read(char* out, size_t size, size_t* nread) {
  if (!out || !size || !nread) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  size_t total = 0;          // how many bytes we've readed
  size_t bytes_left = size;  // how many we have left to read

  while (total < size) {
    size_t n;
    common::Error err = sock_.read(out + total, size, &n);
    if (err && err->IsError()) {
      return err;
    }
    total += n;
    bytes_left -= n;
  }

  *nread = total;  // return number actually readed here
  return common::Error();
}

void TcpClient::CloseImpl() {
  common::Error err = sock_.close();
  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
  }
}

}  // namespace tcp
}
}  // namespace fastotv
}  // namespace fasto
