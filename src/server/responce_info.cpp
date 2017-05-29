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

#include <string>

#include "third-party/json-c/json-c/json.h"  // for json_object_...

#include <common/sprintf.h>
#include <common/convert2string.h>

namespace fasto {
namespace fastotv {
namespace server {

ResponceInfo::ResponceInfo() : request_id(), state(), command(), responce_json() {
}

ResponceInfo::ResponceInfo(const std::string& request_id,
                           const std::string& state_command,
                           const std::string& command,
                           const std::string& responce)
    : request_id(request_id), state(state_command), command(command), responce_json(responce) {
}

json_object* ResponceInfo::MakeJobject(const ResponceInfo& uinfo) {
  json_object* obj = json_object_new_object();

  json_object_object_add(
      obj, RESPONCE_INFO_REQUEST_ID_FIELD, json_object_new_string(uinfo.request_id.c_str()));
  json_object_object_add(
      obj, RESPONCE_INFO_STATE_FIELD, json_object_new_string(uinfo.state.c_str()));
  json_object_object_add(
      obj, RESPONCE_INFO_COMMAND_FIELD, json_object_new_string(uinfo.command.c_str()));
  json_object_object_add(
      obj, RESPONCE_INFO_RESPONCE_FIELD, json_object_new_string(uinfo.responce_json.c_str()));

  return obj;
}

std::string ResponceInfo::ToString() const {
  json_object* obj = MakeJobject(*this);
  std::string str = json_object_get_string(obj);
  json_object_put(obj);
  return str;
}

ResponceInfo ResponceInfo::MakeClass(json_object* obj) {
  if (!obj) {
    return ResponceInfo();
  }

  ResponceInfo inf;
  json_object* jrequest_id = NULL;
  json_bool jrequest_id_exists =
      json_object_object_get_ex(obj, RESPONCE_INFO_REQUEST_ID_FIELD, &jrequest_id);
  if (jrequest_id_exists) {
    inf.request_id = json_object_get_string(jrequest_id);
  }

  json_object* jstate = NULL;
  json_bool jstate_exists = json_object_object_get_ex(obj, RESPONCE_INFO_STATE_FIELD, &jstate);
  if (jstate_exists) {
    inf.state = json_object_get_string(jstate);
  }

  json_object* jcommand = NULL;
  json_bool jcommand_exists =
      json_object_object_get_ex(obj, RESPONCE_INFO_COMMAND_FIELD, &jcommand);
  if (jcommand_exists) {
    inf.command = json_object_get_string(jcommand);
  }

  json_object* jresponce = NULL;
  json_bool jresponce_exists =
      json_object_object_get_ex(obj, RESPONCE_INFO_RESPONCE_FIELD, &jresponce);
  if (jresponce_exists) {
    inf.responce_json = json_object_get_string(jresponce);
  }
  return inf;
}
}
}  // namespace fastotv
}  // namespace fasto
