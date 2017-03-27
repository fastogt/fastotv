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

#include "inner/inner_client.h"

namespace fasto {
namespace fastotv {
namespace inner {

InnerClient::InnerClient(network::tcp::ITcpLoop* server, const common::net::socket_info& info)
    : TcpClient(server, info) {}

const char* InnerClient::ClassName() const {
  return "InnerClient";
}

common::Error InnerClient::Write(const cmd_request_t& request, size_t* nwrite) {
  return WriteInner(request.data(), request.size(), nwrite);
}

common::Error InnerClient::Write(const cmd_responce_t& responce, size_t* nwrite) {
  return WriteInner(responce.data(), responce.size(), nwrite);
}

common::Error InnerClient::Write(const cmd_approve_t& approve, size_t* nwrite) {
  return WriteInner(approve.data(), approve.size(), nwrite);
}

common::Error InnerClient::ReadDataSize(protocoled_size_t* sz) {
  if (!sz) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  protocoled_size_t lsz = 0;
  ssize_t nread;
  common::Error err = Read(reinterpret_cast<char*>(&lsz), sizeof(protocoled_size_t), &nread);
  if (err && err->IsError()) {
    return err;
  }

  *sz = lsz;
  return common::Error();
}

common::Error InnerClient::ReadMessage(char* out, protocoled_size_t size, ssize_t* nread) {
  return Read(out, size, nread);
}

common::Error InnerClient::ReadCommand(std::string* out) {
  if (!out) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  protocoled_size_t message_size;
  common::Error err = ReadDataSize(&message_size);
  if (err && err->IsError()) {
    return err;
  }

  message_size = ntohl(message_size);  // stable
  char* msg = static_cast<char*>(malloc(message_size));
  ssize_t nread;
  err = ReadMessage(msg, message_size, &nread);
  if (err && err->IsError()) {
    free(msg);
    return err;
  }

  *out = std::string(msg, message_size);
  free(msg);
  return common::Error();
}

common::Error InnerClient::WriteInner(const char* data, size_t size, size_t* nwrite) {
  const protocoled_size_t data_size = size;
  const size_t protocoled_data_len = size + sizeof(protocoled_size_t);

  char* protocoled_data = static_cast<char*>(malloc(protocoled_data_len));
  memcpy(protocoled_data, &data_size, sizeof(protocoled_size_t));
  memcpy(protocoled_data + sizeof(protocoled_size_t), data, data_size);
  common::Error err = TcpClient::Write(protocoled_data, protocoled_data_len, nwrite);
  free(protocoled_data);
  return err;
}

}  // namespace inner
}  // namespace fastotv
}  // namespace fasto
