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

#pragma once

#include <string>

namespace fasto {
namespace fastotv {

struct UserAuthInfo {
  UserAuthInfo();
  UserAuthInfo(const std::string& login,
               const std::string& password);

  bool isValid() const;

  std::string login;
  std::string password;
};

inline bool operator==(const UserAuthInfo& lhs, const UserAuthInfo& rhs) {
  return lhs.login == rhs.login && lhs.password == rhs.password;
}

inline bool operator!=(const UserAuthInfo& x, const UserAuthInfo& y) {
  return !(x == y);
}

}  // namespace fastotv
}  // namespace fasto

namespace common {

std::string ConvertToString(const fasto::fastotv::UserAuthInfo& uinfo);

}  // namespace common
