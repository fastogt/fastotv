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

#define RESPONCE_INFO_REQUEST_ID_FIELD "request_id"
#define RESPONCE_INFO_STATE_FIELD "state"
#define RESPONCE_INFO_COMMAND_FIELD "command"
#define RESPONCE_INFO_RESPONCE_FIELD "responce_json"

namespace fasto {
namespace fastotv {
namespace server {

struct ResponceInfo {
  ResponceInfo();
  ResponceInfo(const std::string& request_id,
               const std::string& state_command,
               const std::string& command,
               const std::string& responce);

  static json_object* MakeJobject(const ResponceInfo& url);  // allocate json_object
  static ResponceInfo MakeClass(json_object* obj);           // pass valid json obj

  std::string ToString() const;

  std::string request_id;
  std::string state;
  std::string command;
  std::string responce_json;
};
}
}  // namespace fastotv
}  // namespace fasto
