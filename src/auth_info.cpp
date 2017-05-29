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

struct json_object* AuthInfo::MakeJobject(const AuthInfo& ainf) {
  json_object* obj = json_object_new_object();
  const std::string login_str = ainf.login;
  const std::string password_str = ainf.password;
  json_object_object_add(obj, AUTH_INFO_LOGIN_FIELD, json_object_new_string(login_str.c_str()));
  json_object_object_add(
      obj, AUTH_INFO_PASSWORD_FIELD, json_object_new_string(password_str.c_str()));
  return obj;
}

AuthInfo AuthInfo::MakeClass(json_object* obj) {
  json_object* jlogin = NULL;
  json_bool jlogin_exists = json_object_object_get_ex(obj, AUTH_INFO_LOGIN_FIELD, &jlogin);
  if (!jlogin_exists) {
    return fasto::fastotv::AuthInfo();
  }

  json_object* jpass = NULL;
  json_bool jpass_exists = json_object_object_get_ex(obj, AUTH_INFO_PASSWORD_FIELD, &jpass);
  if (!jpass_exists) {
    return fasto::fastotv::AuthInfo();
  }

  fasto::fastotv::AuthInfo ainf(json_object_get_string(jlogin), json_object_get_string(jpass));
  return ainf;
}

}  // namespace fastotv
}  // namespace fasto
