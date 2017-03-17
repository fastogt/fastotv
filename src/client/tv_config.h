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

#pragma once

#include <utility>
#include <vector>
#include <string>

#include <common/net/types.h>
#include <common/url.h>

#define SETTINGS_SECTION_LABEL "settings"

//
#define LOGIN_SETTING_LABEL "login"
#define PASSWORD_SETTING_LABEL "password"

/*[settings]
login=x
password=y  //md5 hash
*/

namespace fasto {
namespace fastotv {
namespace client {

struct TvConfig {
  std::string login;
  std::string password;
};

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
