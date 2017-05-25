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

#include <common/net/net.h>

struct json_object;

#define BANDWIDTH_HOST_FIELD "bandwidth_host"

namespace fasto {
namespace fastotv {

struct ServerInfo {
  ServerInfo();

  static json_object* MakeJobject(const ServerInfo& inf);  // allocate json_object
  static ServerInfo MakeClass(json_object* obj);           // pass valid json obj

  common::net::HostAndPort bandwidth_host;
};

}  // namespace fastotv
}  // namespace fasto
