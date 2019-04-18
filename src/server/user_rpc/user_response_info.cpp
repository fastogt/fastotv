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

#include "server/user_rpc/user_response_info.h"

#define USER_REQUEST_INFO_RESPONSE_FIELD "response"

namespace fastotv {
namespace server {

UserResponseInfo::UserResponseInfo() : base_class(), resp_() {}

UserResponseInfo::UserResponseInfo(const user_id_t& uid,
                                   const device_id_t& device_id,
                                   const protocol::request_t& req,
                                   const protocol::response_t& resp)
    : base_class(uid, device_id, req), resp_(resp) {}

protocol::response_t UserResponseInfo::GetResponse() const {
  return resp_;
}

bool UserResponseInfo::Equals(const UserResponseInfo& state) const {
  return base_class::Equals(state) && resp_ == state.resp_;
}

common::Error UserResponseInfo::SerializeFields(json_object* deserialized) const {
  common::Error err = base_class::SerializeFields(deserialized);
  if (err) {
    return err;
  }

  json_object* resp_json = nullptr;
  err = common::protocols::json_rpc::MakeJsonRPCResponse(resp_, &resp_json);
  if (err) {
    return err;
  }

  json_object_object_add(deserialized, USER_REQUEST_INFO_RESPONSE_FIELD, resp_json);
  return common::Error();
}

common::Error UserResponseInfo::DoDeSerialize(json_object* serialized) {
  UserResponseInfo inf;
  common::Error err = inf.base_class::DoDeSerialize(serialized);
  if (err) {
    return err;
  }

  json_object* jresp = nullptr;
  json_bool jreq_exists = json_object_object_get_ex(serialized, USER_REQUEST_INFO_RESPONSE_FIELD, &jresp);
  if (jreq_exists) {
    protocol::response_t resp;
    err = common::protocols::json_rpc::ParseJsonRPCResponse(jresp, &resp);
    if (!err) {
      inf.resp_ = resp;
    }
  }

  *this = inf;
  return common::Error();
}

}  // namespace server
}  // namespace fastotv
