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

#include "user_info.h"

#define USER_STATE_INFO_ID_FIELD "user_id"
#define USER_STATE_INFO_CONNECTED_FIELD "connected"

namespace fasto {
namespace fastotv {
namespace server {

struct UserStateInfo {
  UserStateInfo();
  UserStateInfo(const user_id_t& uid, bool connected);

  static json_object* MakeJobject(const UserStateInfo& url);  // allocate json_object
  static UserStateInfo MakeClass(json_object* obj);           // pass valid json obj

  user_id_t user_id;
  bool connected;
};

}
}  // namespace fastotv
}  // namespace fasto
