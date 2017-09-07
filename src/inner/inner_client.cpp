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

#include <common/sys_byteorder.h>

#include <common/text_decoders/compress_snappy_edcoder.h>

namespace fastotv {
namespace inner {

InnerClient::InnerClient(common::libev::IoLoop* server, const common::net::socket_info& info)
    : common::libev::tcp::TcpClient(server, info), compressor_(new common::CompressSnappyEDcoder) {}

InnerClient::~InnerClient() {
  destroy(&compressor_);
}

const char* InnerClient::ClassName() const {
  return "InnerClient";
}

common::Error InnerClient::Write(const cmd_request_t& request) {
  return WriteMessage(request.GetCmd());
}

common::Error InnerClient::Write(const cmd_responce_t& responce) {
  return WriteMessage(responce.GetCmd());
}

common::Error InnerClient::Write(const cmd_approve_t& approve) {
  return WriteMessage(approve.GetCmd());
}

common::Error InnerClient::ReadDataSize(protocoled_size_t* sz) {
  if (!sz) {
    return common::make_inval_error_value(common::ERROR_TYPE);
  }

  protocoled_size_t lsz = 0;
  size_t nread = 0;
  common::Error err = Read(reinterpret_cast<char*>(&lsz), sizeof(protocoled_size_t), &nread);
  if (err && err->IsError()) {
    return err;
  }

  if (nread != sizeof(protocoled_size_t)) {  // connection closed
    if (nread == 0) {
      return common::make_error_value("Connection closed", common::ERROR_TYPE);
    }
    return common::make_error_value(common::MemSPrintf("Error when reading needed to read: %lu bytes, but readed: %lu",
                                                       sizeof(protocoled_size_t), nread),
                                    common::ERROR_TYPE);
  }

  *sz = lsz;
  return common::Error();
}

common::Error InnerClient::ReadMessage(char* out, protocoled_size_t size) {
  if (!out || size == 0) {
    return common::make_inval_error_value(common::ERROR_TYPE);
  }

  size_t nread;
  common::Error err = Read(out, size, &nread);
  if (err && err->IsError()) {
    return err;
  }

  if (nread != size) {  // connection closed
    if (nread == 0) {
      return common::make_error_value("Connection closed", common::ERROR_TYPE);
    }
    return common::make_error_value(
        common::MemSPrintf("Error when reading needed to read: %lu bytes, but readed: %lu", size, nread),
        common::ERROR_TYPE);
  }

  return common::Error();
}

common::Error InnerClient::ReadCommand(std::string* out) {
  if (!out) {
    return common::make_inval_error_value(common::ERROR_TYPE);
  }

  protocoled_size_t message_size;
  common::Error err = ReadDataSize(&message_size);
  if (err && err->IsError()) {
    return err;
  }

  message_size = common::NetToHost32(message_size);  // stable
  char* msg = static_cast<char*>(malloc(message_size));
  err = ReadMessage(msg, message_size);
  if (err && err->IsError()) {
    free(msg);
    return err;
  }

  const common::StringPiece compressed(msg, message_size);
  std::string un_compressed;
  err = compressor_->Decode(compressed, &un_compressed);
  free(msg);
  if (err && err->IsError()) {
    return err;
  }

  *out = un_compressed;
  return common::Error();
}

common::Error InnerClient::WriteMessage(const std::string& message) {
  if (message.empty()) {
    return common::make_inval_error_value(common::ERROR_TYPE);
  }

  std::string compressed;
  common::Error err = compressor_->Encode(message, &compressed);
  if (err && err->IsError()) {
    return err;
  }

  const char* data_ptr = compressed.data();
  const size_t size = compressed.size();

  const protocoled_size_t data_size = size;
  const protocoled_size_t message_size = common::HostToNet32(data_size);  // stabled
  const size_t protocoled_data_len = size + sizeof(protocoled_size_t);

  char* protocoled_data = static_cast<char*>(malloc(protocoled_data_len));
  memcpy(protocoled_data, &message_size, sizeof(protocoled_size_t));
  memcpy(protocoled_data + sizeof(protocoled_size_t), data_ptr, data_size);
  size_t nwrite = 0;
  err = TcpClient::Write(protocoled_data, protocoled_data_len, &nwrite);
  if (nwrite != protocoled_data_len) {  // connection closed
    free(protocoled_data);
    return common::make_error_value(
        common::MemSPrintf("Error when writing needed to write: %lu, but writed: %lu", protocoled_data_len, nwrite),
        common::ERROR_TYPE);
  }

  free(protocoled_data);
  return err;
}

}  // namespace inner
}  // namespace fastotv
