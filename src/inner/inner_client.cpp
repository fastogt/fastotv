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

common::ErrnoError InnerClient::Write(const common::protocols::three_way_handshake::cmd_request_t& request) {
  return WriteMessage(request.GetCmd());
}

common::ErrnoError InnerClient::Write(const common::protocols::three_way_handshake::cmd_responce_t& responce) {
  return WriteMessage(responce.GetCmd());
}

common::ErrnoError InnerClient::Write(const common::protocols::three_way_handshake::cmd_approve_t& approve) {
  return WriteMessage(approve.GetCmd());
}

common::ErrnoError InnerClient::ReadDataSize(protocoled_size_t* sz) {
  if (!sz) {
    return common::make_errno_error_inval();
  }

  protocoled_size_t lsz = 0;
  size_t nread = 0;
  common::ErrnoError err = Read(reinterpret_cast<char*>(&lsz), sizeof(protocoled_size_t), &nread);
  if (err) {
    return err;
  }

  if (nread != sizeof(protocoled_size_t)) {  // connection closed
    if (nread == 0) {
      return common::make_errno_error("Connection closed", ECONNRESET);
    }
    return common::make_errno_error(common::MemSPrintf("Error when reading needed to read: %lu bytes, but readed: %lu",
                                                       sizeof(protocoled_size_t), nread),
                                    EINVAL);
  }

  *sz = lsz;
  return common::ErrnoError();
}

common::ErrnoError InnerClient::ReadMessage(char* out, protocoled_size_t size) {
  if (!out || size == 0) {
    return common::make_errno_error_inval();
  }

  size_t nread;
  common::ErrnoError err = Read(out, size, &nread);
  if (err) {
    return err;
  }

  if (nread != size) {  // connection closed
    if (nread == 0) {
      return common::make_errno_error("Connection closed", ECONNRESET);
    }
    return common::make_errno_error(
        common::MemSPrintf("Error when reading needed to read: %lu bytes, but readed: %lu", size, nread), EINVAL);
  }

  return common::ErrnoError();
}

common::ErrnoError InnerClient::ReadCommand(std::string* out) {
  if (!out) {
    return common::make_errno_error_inval();
  }

  protocoled_size_t message_size;
  common::ErrnoError err = ReadDataSize(&message_size);
  if (err) {
    return err;
  }

  message_size = common::NetToHost32(message_size);  // stable
  if (message_size > MAX_COMMAND_SIZE) {
    return common::make_errno_error(common::MemSPrintf("Reached limit of command size: %u", message_size), EINVAL);
  }

  char* msg = static_cast<char*>(malloc(message_size));
  err = ReadMessage(msg, message_size);
  if (err) {
    free(msg);
    return err;
  }

  const common::StringPiece compressed(msg, message_size);
  std::string un_compressed;
  common::Error dec_err = compressor_->Decode(compressed, &un_compressed);
  free(msg);
  if (dec_err) {
    return common::make_errno_error(dec_err->GetDescription(), EINVAL);
  }

  *out = un_compressed;
  return common::ErrnoError();
}

common::ErrnoError InnerClient::WriteMessage(const std::string& message) {
  if (message.empty()) {
    return common::make_errno_error_inval();
  }

  std::string compressed;
  common::Error enc_err = compressor_->Encode(message, &compressed);
  if (enc_err) {
    return common::make_errno_error(enc_err->GetDescription(), EINVAL);
  }

  const char* data_ptr = compressed.data();
  const size_t size = compressed.size();

  if (size > MAX_COMMAND_SIZE) {
    return common::make_errno_error(common::MemSPrintf("Reached limit of command size: %u", size), EINVAL);
  }

  const protocoled_size_t message_size = common::HostToNet32(size);  // stable
  const size_t protocoled_data_len = size + sizeof(protocoled_size_t);

  char* protocoled_data = static_cast<char*>(malloc(protocoled_data_len));
  memcpy(protocoled_data, &message_size, sizeof(protocoled_size_t));
  memcpy(protocoled_data + sizeof(protocoled_size_t), data_ptr, size);
  size_t nwrite = 0;
  common::ErrnoError err = TcpClient::Write(protocoled_data, protocoled_data_len, &nwrite);
  if (nwrite != protocoled_data_len) {  // connection closed
    free(protocoled_data);
    return common::make_errno_error(
        common::MemSPrintf("Error when writing needed to write: %lu, but writed: %lu", protocoled_data_len, nwrite),
        EINVAL);
  }

  free(protocoled_data);
  return err;
}

}  // namespace inner
}  // namespace fastotv
