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
#include <vector>

#include "url.h"

namespace fasto {
namespace fastotv {

struct AuthInfo {
  AuthInfo();
  AuthInfo(const std::string& login, const std::string& password);

  bool IsValid() const;

  static json_object* MakeJobject(const AuthInfo& ainf);  // allocate json_object
  static AuthInfo MakeClass(json_object* obj);            // pass valid json obj

  std::string login;  // unique
  std::string password;
};

inline bool operator==(const AuthInfo& lhs, const AuthInfo& rhs) {
  return lhs.login == rhs.login && lhs.password == rhs.password;
}

inline bool operator!=(const AuthInfo& x, const AuthInfo& y) {
  return !(x == y);
}

typedef std::vector<Url> channels_t;

json_object* MakeJobjectFromChannels(const channels_t& channels);  // allocate json_object
channels_t MakeChannelsClass(json_object* obj);                    // pass valid json obj

struct UserInfo {
  UserInfo();
  explicit UserInfo(const AuthInfo& a, const std::vector<Url>& ch);

  bool IsValid() const;
  std::string GetLogin() const;
  std::string GetPassword() const;

  static json_object* MakeJobject(const UserInfo& url);  // allocate json_object
  static UserInfo MakeClass(json_object* obj);           // pass valid json obj

  AuthInfo auth;
  channels_t channels;
};

inline bool operator==(const UserInfo& lhs, const UserInfo& rhs) {
  return lhs.auth == rhs.auth;
}

inline bool operator!=(const UserInfo& x, const UserInfo& y) {
  return !(x == y);
}

}  // namespace fastotv
}  // namespace fasto
