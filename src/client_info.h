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

#include <string>

#include "client_server_types.h"

struct json_object;

#define CLIENT_INFO_LOGIN_FIELD "login"
#define CLIENT_INFO_BANDWIDTH_FIELD "bandwidth"
#define CLIENT_INFO_OS_FIELD "os"
#define CLIENT_INFO_CPU_FIELD "cpu"
#define CLIENT_INFO_RAM_TOTAL_FIELD "ram_total"
#define CLIENT_INFO_RAM_FREE_FIELD "ram_free"

namespace fasto {
namespace fastotv {

struct ClientInfo {
  ClientInfo();
  bool IsValid() const;

  static json_object* MakeJobject(const ClientInfo& inf);  // allocate json_object
  static ClientInfo MakeClass(json_object* obj);           // pass valid json obj

  login_t login;
  std::string os;
  std::string cpu_brand;
  int64_t ram_total;
  int64_t ram_free;
  bandwidth_t bandwidth;
};

}  // namespace fastotv
}  // namespace fasto
