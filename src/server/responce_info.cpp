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

#include "server/responce_info.h"

#include <stddef.h>  // for NULL
#include <string>    // for string

#include <json-c/json_object.h>  // for json_object, json...

#define RESPONCE_INFO_REQUEST_ID_FIELD "request_id"
#define RESPONCE_INFO_STATE_FIELD "state"
#define RESPONCE_INFO_COMMAND_FIELD "command"
#define RESPONCE_INFO_RESPONCE_FIELD "responce_json"

namespace fastotv {
namespace server {

ResponceInfo::ResponceInfo() : request_id_(), state_(), command_(), responce_json_() {}

ResponceInfo::ResponceInfo(const std::string& request_id,
                           const std::string& state_command,
                           const std::string& command,
                           const std::string& responce)
    : request_id_(request_id), state_(state_command), command_(command), responce_json_(responce) {}

common::Error ResponceInfo::SerializeFields(json_object* obj) const {
  json_object_object_add(obj, RESPONCE_INFO_REQUEST_ID_FIELD, json_object_new_string(request_id_.c_str()));
  json_object_object_add(obj, RESPONCE_INFO_STATE_FIELD, json_object_new_string(state_.c_str()));
  json_object_object_add(obj, RESPONCE_INFO_COMMAND_FIELD, json_object_new_string(command_.c_str()));
  json_object_object_add(obj, RESPONCE_INFO_RESPONCE_FIELD, json_object_new_string(responce_json_.c_str()));
  return common::Error();
}

common::Error ResponceInfo::DeSerialize(const serialize_type& serialized, ResponceInfo* obj) {
  if (!serialized || !obj) {
    return common::make_error_inval();
  }

  ResponceInfo inf;
  json_object* jrequest_id = NULL;
  json_bool jrequest_id_exists = json_object_object_get_ex(serialized, RESPONCE_INFO_REQUEST_ID_FIELD, &jrequest_id);
  if (jrequest_id_exists) {
    inf.request_id_ = json_object_get_string(jrequest_id);
  }

  json_object* jstate = NULL;
  json_bool jstate_exists = json_object_object_get_ex(serialized, RESPONCE_INFO_STATE_FIELD, &jstate);
  if (jstate_exists) {
    inf.state_ = json_object_get_string(jstate);
  }

  json_object* jcommand = NULL;
  json_bool jcommand_exists = json_object_object_get_ex(serialized, RESPONCE_INFO_COMMAND_FIELD, &jcommand);
  if (jcommand_exists) {
    inf.command_ = json_object_get_string(jcommand);
  }

  json_object* jresponce = NULL;
  json_bool jresponce_exists = json_object_object_get_ex(serialized, RESPONCE_INFO_RESPONCE_FIELD, &jresponce);
  if (jresponce_exists) {
    inf.responce_json_ = json_object_get_string(jresponce);
  }

  *obj = inf;
  return common::Error();
}

std::string ResponceInfo::GetRequestId() const {
  return request_id_;
}

std::string ResponceInfo::GetState() const {
  return state_;
}

std::string ResponceInfo::GetCommand() const {
  return command_;
}

std::string ResponceInfo::GetResponceJson() const {
  return responce_json_;
}

bool ResponceInfo::Equals(const ResponceInfo& inf) const {
  return request_id_ == inf.request_id_ && state_ == inf.state_ && command_ == inf.command_ &&
         responce_json_ == inf.responce_json_;
}

}  // namespace server
}  // namespace fastotv
