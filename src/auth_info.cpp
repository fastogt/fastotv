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

#include "auth_info.h"

#include <string>

#include "third-party/json-c/json-c/json.h"  // for json_object_...

#include <common/sprintf.h>
#include <common/convert2string.h>

namespace fasto {
namespace fastotv {

AuthInfo::AuthInfo() : login(), password() {
}

AuthInfo::AuthInfo(const std::string& login, const std::string& password)
    : login(login), password(password) {
}

bool AuthInfo::IsValid() const {
  return !login.empty();
}

common::Error AuthInfo::Serialize(serialize_type* deserialized) const {
  json_object* obj = json_object_new_object();
  json_object_object_add(obj, AUTH_INFO_LOGIN_FIELD, json_object_new_string(login.c_str()));
  json_object_object_add(obj, AUTH_INFO_PASSWORD_FIELD, json_object_new_string(password.c_str()));

  *deserialized = obj;
  return common::Error();
}

common::Error AuthInfo::DeSerialize(const serialize_type& serialized, value_type* obj) {
  json_object* jlogin = NULL;
  json_bool jlogin_exists = json_object_object_get_ex(serialized, AUTH_INFO_LOGIN_FIELD, &jlogin);
  if (!jlogin_exists) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  json_object* jpass = NULL;
  json_bool jpass_exists = json_object_object_get_ex(serialized, AUTH_INFO_PASSWORD_FIELD, &jpass);
  if (!jpass_exists) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  fasto::fastotv::AuthInfo ainf(json_object_get_string(jlogin), json_object_get_string(jpass));
  *obj = ainf;
  return common::Error();
}

}  // namespace fastotv
}  // namespace fasto
