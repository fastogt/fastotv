/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of SiteOnYourDevice.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "infos.h"

#include <string>

#include <common/sprintf.h>
#include <common/convert2string.h>

namespace fasto {
namespace fastotv {

UserAuthInfo::UserAuthInfo() : login(), password() {}

UserAuthInfo::UserAuthInfo(const std::string& login, const std::string& password)
    : login(login), password(password) {}

bool UserAuthInfo::isValid() const {
  return !login.empty();
}

}  // namespace fastotv
}  // namespace fasto

namespace common {

std::string ConvertToString(const fasto::fastotv::UserAuthInfo& uinfo) {
  return common::MemSPrintf("%s:%s", uinfo.login, uinfo.password);
}

template <>
fasto::fastotv::UserAuthInfo ConvertFromString(const std::string& uinfo_str) {
  size_t up = uinfo_str.find_first_of(':');
  if (up == std::string::npos) {
    return fasto::fastotv::UserAuthInfo();
  }

  size_t ph = uinfo_str.find_first_of(':', up + 1);
  if (ph == std::string::npos) {
    return fasto::fastotv::UserAuthInfo();
  }

  fasto::fastotv::UserAuthInfo uinfo;
  uinfo.login = uinfo_str.substr(0, up);
  uinfo.password = uinfo_str.substr(up + 1, ph - up - 1);

  return uinfo;
}

}  // namespace common
