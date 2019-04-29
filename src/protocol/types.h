/*  Copyright (C) 2014-2019 FastoGT. All right reserved.
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

#pragma once

#include <string>

#include <common/protocols/json_rpc/json_rpc.h>

#define OK_RESULT "OK"

namespace fastotv {
namespace protocol {

typedef common::protocols::json_rpc::JsonRPCResponse response_t;
typedef common::protocols::json_rpc::JsonRPCRequest request_t;
typedef uint64_t seq_id_t;
typedef common::protocols::json_rpc::json_rpc_id sequance_id_t;
typedef common::protocols::json_rpc::json_rpc_request_params serializet_params_t;

common::protocols::json_rpc::JsonRPCMessage MakeSuccessMessage(const std::string& result = OK_RESULT);
common::protocols::json_rpc::JsonRPCError MakeServerErrorFromText(const std::string& error_text);
common::protocols::json_rpc::JsonRPCError MakeInternalErrorFromText(const std::string& error_text);

sequance_id_t MakeRequestID(seq_id_t sid);

}  // namespace protocol
}  // namespace fastotv
