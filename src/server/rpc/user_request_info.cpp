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

#include "server/rpc/user_request_info.h"

#define USER_REQUEST_INFO_REQUEST_FIELD "request"

namespace fastotv {
namespace server {
namespace rpc {

UserRequestInfo::UserRequestInfo() : base_class(), req_() {}

UserRequestInfo::UserRequestInfo(const user_id_t& uid, const device_id_t& device_id, const protocol::request_t& req)
    : base_class(uid, device_id), req_(req) {}

protocol::request_t UserRequestInfo::GetRequest() const {
  return req_;
}

bool UserRequestInfo::Equals(const UserRequestInfo& state) const {
  return base_class::Equals(state) && req_ == state.req_;
}

common::Error UserRequestInfo::SerializeFields(json_object* deserialized) const {
  common::Error err = base_class::SerializeFields(deserialized);
  if (err) {
    return err;
  }

  json_object* req_json = nullptr;
  err = common::protocols::json_rpc::MakeJsonRPCRequest(req_, &req_json);
  if (err) {
    return err;
  }
  json_object_object_add(deserialized, USER_REQUEST_INFO_REQUEST_FIELD, req_json);
  return common::Error();
}

common::Error UserRequestInfo::DoDeSerialize(json_object* serialized) {
  UserRequestInfo inf;
  common::Error err = inf.base_class::DoDeSerialize(serialized);
  if (err) {
    return err;
  }

  json_object* jreq = nullptr;
  json_bool jreq_exists = json_object_object_get_ex(serialized, USER_REQUEST_INFO_REQUEST_FIELD, &jreq);
  if (jreq_exists) {
    protocol::request_t req;
    err = common::protocols::json_rpc::ParseJsonRPCRequest(jreq, &req);
    if (!err) {
      inf.req_ = req;
    }
  }

  *this = inf;
  return common::Error();
}

}  // namespace rpc
}  // namespace server
}  // namespace fastotv
