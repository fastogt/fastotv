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

#include "server/config.h"

#include <string.h>  // for strcmp

#include <common/logger.h>  // for COMPACT_LOG_WARNING, WARNING_LOG

#include "inih/ini.h"

#define CHANNEL_COMMANDS_IN_NAME "COMMANDS_IN"
#define CHANNEL_COMMANDS_OUT_NAME "COMMANDS_OUT"
#define CHANNEL_CLIENTS_STATE_NAME "CLIENTS_STATE"

#define CONFIG_SERVER_OPTIONS "server"
#define CONFIG_SERVER_OPTIONS_HOST_FIELD "host"
#define CONFIG_SERVER_OPTIONS_REDIS_SERVER_FIELD "redis_server"
#define CONFIG_SERVER_OPTIONS_REDIS_UNIX_PATH_FIELD "redis_unix_path"
#define CONFIG_SERVER_OPTIONS_REDIS_CHANNEL_IN_FIELD "redis_channel_in_name"
#define CONFIG_SERVER_OPTIONS_REDIS_CHANNEL_OUT_FIELD "redis_channel_out_name"
#define CONFIG_SERVER_OPTIONS_REDIS_CHANNEL_STATUS_FIELD "redis_channel_clients_state_name"
#define CONFIG_SERVER_OPTIONS_BANDWIDT_SERVER_FIELD "bandwidth_server"

/*
  [server]
  host=fastotv.com:7040
  redis_server=localhost:6379
  redis_unix_path=/var/run/redis/redis.sock
  bandwidth_server=localhost:5544
*/

namespace fastotv {
namespace server {
namespace {
int ini_handler_fasto(void* user_data, const char* section, const char* name, const char* value) {
  Config* pconfig = reinterpret_cast<Config*>(user_data);

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
  if (MATCH(CONFIG_SERVER_OPTIONS, CONFIG_SERVER_OPTIONS_REDIS_SERVER_FIELD)) {
    common::net::HostAndPort hs;
    bool res = common::ConvertFromString(value, &hs);
    if (!res) {
      WARNING_LOG() << "Invalid " CONFIG_SERVER_OPTIONS_REDIS_SERVER_FIELD " value: " << value;
      return 0;
    }
    pconfig->server.redis.redis_host = hs;
    return 1;
  } else if (MATCH(CONFIG_SERVER_OPTIONS, CONFIG_SERVER_OPTIONS_HOST_FIELD)) {
    common::net::HostAndPort hs;
    bool res = common::ConvertFromString(value, &hs);
    if (!res) {
      WARNING_LOG() << "Invalid " CONFIG_SERVER_OPTIONS_HOST_FIELD " value: " << value;
      return 0;
    }
    pconfig->server.host = hs;
    return 1;
  } else if (MATCH(CONFIG_SERVER_OPTIONS, CONFIG_SERVER_OPTIONS_REDIS_UNIX_PATH_FIELD)) {
    pconfig->server.redis.redis_unix_socket = value;
    return 1;
  } else if (MATCH(CONFIG_SERVER_OPTIONS, CONFIG_SERVER_OPTIONS_REDIS_CHANNEL_IN_FIELD)) {
    pconfig->server.redis.channel_in = value;
    return 1;
  } else if (MATCH(CONFIG_SERVER_OPTIONS, CONFIG_SERVER_OPTIONS_REDIS_CHANNEL_OUT_FIELD)) {
    pconfig->server.redis.channel_out = value;
    return 1;
  } else if (MATCH(CONFIG_SERVER_OPTIONS, CONFIG_SERVER_OPTIONS_REDIS_CHANNEL_STATUS_FIELD)) {
    pconfig->server.redis.channel_clients_state = value;
    return 1;
  } else if (MATCH(CONFIG_SERVER_OPTIONS, CONFIG_SERVER_OPTIONS_BANDWIDT_SERVER_FIELD)) {
    common::net::HostAndPort hs;
    bool res = common::ConvertFromString(value, &hs);
    if (!res) {
      WARNING_LOG() << "Invalid " CONFIG_SERVER_OPTIONS_BANDWIDT_SERVER_FIELD " value: " << value;
      return 0;
    }
    pconfig->server.bandwidth_host = hs;
    return 1;
  } else {
    return 0; /* unknown section/name, error */
  }
}
}  // namespace

ServerSettings::ServerSettings() : host(), redis(), bandwidth_host() {
  // in config by default
  // redis.redis_host = redis_default_host;
  // redis.redis_unix_socket = redis_default_unix_path;

  redis.channel_in = CHANNEL_COMMANDS_IN_NAME;
  redis.channel_out = CHANNEL_COMMANDS_OUT_NAME;
  redis.channel_clients_state = CHANNEL_CLIENTS_STATE_NAME;

  // bandwidth_host = bandwidth_default_host;
}

Config::Config() : server() {}

common::Error load_config_file(const std::string& config_absolute_path, Config* options) {
  if (!options || config_absolute_path.empty()) {
    return common::make_error_inval();
  }

  ini_parse(config_absolute_path.c_str(), ini_handler_fasto, options);
  return common::Error();
}

common::Error save_config_file(const std::string& config_absolute_path, Config* options) {
  if (!options || config_absolute_path.empty()) {
    return common::make_error_inval();
  }

  return common::Error();
}

}  // namespace server
}  // namespace fastotv
