/*  Copyright (C) 2014-2020 FastoGT. All right reserved.

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

#include <common/error.h>  // for Error
#include <common/net/types.h>

#include <player/tv_config.h>

#include <fastotv/commands_info/auth_info.h>

namespace fastotv {
namespace client {

struct FastoTVConfig : public fastoplayer::TVConfig {
  commands_info::AuthInfo auth_options;
  common::net::HostAndPort server;
};

common::ErrnoError load_config_file(const std::string& config_absolute_path, FastoTVConfig* options) WARN_UNUSED_RESULT;
common::ErrnoError save_config_file(const std::string& config_absolute_path, FastoTVConfig* options) WARN_UNUSED_RESULT;

}  // namespace client
}  // namespace fastotv
