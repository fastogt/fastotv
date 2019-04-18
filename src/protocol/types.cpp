/*  Copyright (C) 2014-2018 FastoGT. All right reserved.
    This file is part of iptv_cloud.
    iptv_cloud is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    iptv_cloud is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with iptv_cloud.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "protocol/types.h"

#include <string.h>

#include <common/convert2string.h>
#include <common/sys_byteorder.h>

namespace fastotv {
namespace protocol {

common::protocols::json_rpc::JsonRPCMessage MakeSuccessMessage(const std::string& result) {
  common::protocols::json_rpc::JsonRPCMessage msg;
  msg.result = result;
  return msg;
}

common::protocols::json_rpc::JsonRPCError MakeServerErrorFromText(const std::string& error_text) {
  common::protocols::json_rpc::JsonRPCError err;
  err.code = common::protocols::json_rpc::JSON_RPC_SERVER_ERROR;
  err.message = error_text;
  return err;
}

common::protocols::json_rpc::JsonRPCError MakeInternalErrorFromText(const std::string& error_text) {
  common::protocols::json_rpc::JsonRPCError err;
  err.code = common::protocols::json_rpc::JSON_RPC_INTERNAL_ERROR;
  err.message = error_text;
  return err;
}

sequance_id_t MakeRequestID(seq_id_t sid) {
  char bytes[sizeof(seq_id_t)];
  const seq_id_t stabled = common::NetToHost64(sid);  // for human readable hex
  memcpy(&bytes, &stabled, sizeof(seq_id_t));
  protocol::sequance_id_t::value_type hexed;
  common::utils::hex::encode(std::string(bytes, sizeof(seq_id_t)), true, &hexed);
  return hexed;
}

}  // namespace protocol
}  // namespace fastotv
