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

#include "redis/redis_helpers.h"

namespace fasto {
namespace fastotv {
namespace server {

struct Settings {
  Settings();

  common::net::HostAndPort host;
  redis_sub_configuration_t redis;
  common::net::HostAndPort bandwidth_host;
};

struct Config {
  Config();

  Settings server;
};

bool load_config_file(const std::string& config_absolute_path, Config* options);
bool save_config_file(const std::string& config_absolute_path, Config* options);

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
