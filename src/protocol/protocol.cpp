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

#include "protocol/protocol.h"

#include <string>

#include <common/sprintf.h>
#include <common/sys_byteorder.h>
#include <common/text_decoders/compress_zlib_edcoder.h>

namespace fastotv {
namespace protocol {

namespace {

common::ErrnoError GenerateProtocoledMessage(const common::char_buffer_t& message, char** data, size_t* data_len) {
  if (message.empty() || !data || !data_len) {
    return common::make_errno_error_inval();
  }

  const char* data_ptr = message.data();
  const size_t size = message.size();

  const protocoled_size_t data_size = size;
  if (data_size > MAX_COMMAND_LENGTH) {
    return common::make_errno_error(common::MemSPrintf("Reached limit of command size: %u", data_size), EAGAIN);
  }

  const protocoled_size_t message_size = common::HostToNet32(data_size);  // stable
  const size_t protocoled_data_len = size + sizeof(protocoled_size_t);

  char* protocoled_data = static_cast<char*>(malloc(protocoled_data_len));
  if (!protocoled_data) {
    return common::make_errno_error("Can't allocate memory", ENOMEM);
  }

  memcpy(protocoled_data, &message_size, sizeof(protocoled_size_t));
  memcpy(protocoled_data + sizeof(protocoled_size_t), data_ptr, data_size);

  *data = protocoled_data;
  *data_len = protocoled_data_len;
  return common::ErrnoError();
}
}  // namespace

namespace detail {
namespace {
common::ErrnoError ReadDataSize(common::libev::IoClient* client, protocoled_size_t* sz) {
  if (!client || !sz) {
    return common::make_errno_error_inval();
  }

  protocoled_size_t lsz = 0;
  size_t nread = 0;
  common::ErrnoError err = client->Read(reinterpret_cast<char*>(&lsz), sizeof(protocoled_size_t), &nread);
  if (err) {
    return err;
  }

  if (nread != sizeof(protocoled_size_t)) {  // connection closed
    if (nread == 0) {
      return common::make_errno_error("Connection closed", EAGAIN);
    }
    return common::make_errno_error(common::MemSPrintf("Error when reading needed to read: %lu bytes, but readed: %lu",
                                                       sizeof(protocoled_size_t), nread),
                                    EAGAIN);
  }

  *sz = lsz;
  return common::ErrnoError();
}

common::ErrnoError ReadMessage(common::libev::IoClient* client, char* out, protocoled_size_t size) {
  if (!client || !out || size == 0) {
    return common::make_errno_error_inval();
  }

  size_t nread;
  common::ErrnoError err = client->Read(out, size, &nread);
  if (err) {
    return err;
  }

  if (nread != size) {  // connection closed
    if (nread == 0) {
      return common::make_errno_error("Connection closed", EAGAIN);
    }
    return common::make_errno_error(
        common::MemSPrintf("Error when reading needed to read: %lu bytes, but readed: %lu", size, nread), EAGAIN);
  }

  return common::ErrnoError();
}
}  // namespace

common::ErrnoError ReadCommand(common::libev::IoClient* client, common::IEDcoder* compressor, std::string* out) {
  if (!client || !compressor || !out) {
    return common::make_errno_error_inval();
  }

  protocoled_size_t message_size;
  common::ErrnoError err = ReadDataSize(client, &message_size);
  if (err) {
    return err;
  }

  message_size = common::NetToHost32(message_size);  // stable
  if (message_size > MAX_COMMAND_LENGTH) {
    return common::make_errno_error(common::MemSPrintf("Reached limit of command size: %u", message_size), EAGAIN);
  }

  char* msg = static_cast<char*>(malloc(message_size));
  err = ReadMessage(client, msg, message_size);
  if (err) {
    free(msg);
    return err;
  }

  const common::char_buffer_t compressed = MAKE_CHAR_BUFFER_SIZE(msg, message_size);
  common::char_buffer_t un_compressed;
  common::Error dec_err = compressor->Decode(compressed, &un_compressed);
  free(msg);
  if (dec_err) {
    return common::make_errno_error(dec_err->GetDescription(), EINVAL);
  }

  *out = un_compressed.as_string();
  return common::ErrnoError();
}

common::ErrnoError WriteMessage(common::libev::IoClient* client,
                                common::IEDcoder* compressor,
                                const std::string& message) {
  if (!client || !compressor || message.empty()) {
    return common::make_errno_error_inval();
  }

  common::char_buffer_t compressed;
  common::Error enc_err = compressor->Encode(message, &compressed);
  if (enc_err) {
    return common::make_errno_error(enc_err->GetDescription(), EINVAL);
  }

  char* protocoled_data = nullptr;
  size_t protocoled_data_len = 0;
  common::ErrnoError err = protocol::GenerateProtocoledMessage(compressed, &protocoled_data, &protocoled_data_len);
  if (err) {
    return err;
  }

  size_t nwrite = 0;
  err = client->Write(protocoled_data, protocoled_data_len, &nwrite);
  free(protocoled_data);
  if (nwrite != protocoled_data_len) {  // connection closed
    return common::make_errno_error(
        common::MemSPrintf("Error when writing needed to write: %lu, but writed: %lu", protocoled_data_len, nwrite),
        EAGAIN);
  }

  return err;
}

common::ErrnoError WriteRequest(common::libev::IoClient* client,
                                common::IEDcoder* compressor,
                                const request_t& request) {
  std::string request_str;
  common::Error err = common::protocols::json_rpc::MakeJsonRPCRequest(request, &request_str);
  if (err) {
    return common::make_errno_error(err->GetDescription(), err->GetErrorCode());
  }
  return WriteMessage(client, compressor, request_str);
}

common::ErrnoError WriteResponce(common::libev::IoClient* client,
                                 common::IEDcoder* compressor,
                                 const response_t& responce) {
  std::string responce_str;
  common::Error err = common::protocols::json_rpc::MakeJsonRPCResponse(responce, &responce_str);
  if (err) {
    return common::make_errno_error(err->GetDescription(), err->GetErrorCode());
  }
  return WriteMessage(client, compressor, responce_str);
}

common::IEDcoder* MakeCompressor() {
  return new common::CompressZlibEDcoder(false, common::CompressZlibEDcoder::GZIP_DEFLATE);
}
}  // namespace detail

}  // namespace protocol
}  // namespace fastotv
