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

#include <signal.h>
#include <unistd.h>

#include <string>
#include <iostream>

#include "inih/ini.h"

#include <common/logger.h>
#include <common/utils.h>
#include <common/file_system.h>
#include <common/convert2string.h>

#include "server_host.h"
#include "server_config.h"

#define MAXLINE 80
#define SBUF_SIZE 256
#define BUF_SIZE 4096

#define HOST_PATH "/"
#define CHANNEL_COMMANDS_IN_NAME "COMMANDS_IN"
#define CHANNEL_COMMANDS_OUT_NAME "COMMANDS_OUT"
#define CHANNEL_CLIENTS_STATE_NAME "CLIENTS_STATE"

/*
  [server]
  redis_server=localhost:6379
  redis_unix_path=/var/run/redis/redis.sock
*/

const common::net::HostAndPort redis_default_host = common::net::HostAndPort::createLocalHost(6379);
const std::string redis_default_unix_path = "/var/run/redis/redis.sock";

sig_atomic_t is_stop = 0;
const char* config_path = SERVER_CONFIG_FILE_PATH;

fasto::fastotv::server::Config config;

int ini_handler_fasto(void* user, const char* section, const char* name, const char* value) {
  fasto::fastotv::server::Config* pconfig = reinterpret_cast<fasto::fastotv::server::Config*>(user);

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
  if (MATCH("server", "redis_server")) {
    common::net::HostAndPort hs;
    bool res = common::ConvertFromString(value, &hs);
    if (!res) {
      WARNING_LOG() << "Invalid host value: " << value;
      return 0;
    }
    pconfig->server.redis.redis_host = hs;
    return 1;
  } else if (MATCH("server", "redis_unix_path")) {
    pconfig->server.redis.redis_unix_socket = value;
    return 1;
  } else if (MATCH("server", "redis_channel_in_name")) {
    pconfig->server.redis.channel_in = value;
    return 1;
  } else if (MATCH("server", "redis_channel_out_name")) {
    pconfig->server.redis.channel_out = value;
    return 1;
  } else if (MATCH("server", "redis_channel_clients_state_name")) {
    pconfig->server.redis.channel_clients_state = value;
    return 1;
  } else {
    return 0; /* unknown section/name, error */
  }
}

static fasto::fastotv::server::ServerHost* server = NULL;

void signal_handler(int sig);
void sync_config();

int main(int argc, char* argv[]) {
  int opt;
  bool daemon_mode = false;
  while ((opt = getopt(argc, argv, "cd:")) != -1) {
    switch (opt) {
      case 'c':
        config_path = argv[optind];
        break;
      case 'd':
        daemon_mode = true;
        break;
      default: /* '?' */
        fprintf(stderr, "Usage: %s [-c] config path [-d] run as daemon\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

#ifdef OS_POSIX
  if (daemon_mode) {
    common::create_as_daemon();
  }
  signal(SIGHUP, signal_handler);
  signal(SIGQUIT, signal_handler);
#endif

#if defined(NDEBUG)
  common::logging::LEVEL_LOG level = common::logging::L_INFO;
#else
  common::logging::LEVEL_LOG level = common::logging::L_DEBUG;
#endif
#if defined(LOG_TO_FILE)
  std::string log_path =
      common::file_system::prepare_path("/var/log/" PROJECT_NAME_LOWERCASE ".log");
  INIT_LOGGER(PROJECT_NAME_TITLE, log_path, level);
#else
  INIT_LOGGER(PROJECT_NAME_TITLE, level);
#endif
  server = new fasto::fastotv::server::ServerHost(fasto::fastotv::g_service_host);

  sync_config();

  common::Error err = common::file_system::change_directory(HOST_PATH);
  int return_code = EXIT_FAILURE;
  if (err && err->IsError()) {
    goto exit;
  }

  return_code = server->exec();

exit:
  delete server;
  return return_code;
}

#ifdef OS_POSIX
void signal_handler(int sig) {
  if (sig == SIGHUP) {
    sync_config();
  } else if (sig == SIGQUIT) {
    is_stop = 1;
    server->stop();
  }
}
#endif

void sync_config() {
  /*{
    "user": "atopilski@gmail.com",
    "password": "1234"
  }*/

  // config.server.redis.redis_host = redis_default_host;
  // config.server.redis.redis_unix_socket = redis_default_unix_path;
  config.server.redis.channel_in = CHANNEL_COMMANDS_IN_NAME;
  config.server.redis.channel_out = CHANNEL_COMMANDS_OUT_NAME;
  config.server.redis.channel_clients_state = CHANNEL_CLIENTS_STATE_NAME;
  // try to parse settings file
  if (ini_parse(config_path, ini_handler_fasto, &config) < 0) {
    INFO_LOG() << "Can't load config " << config_path << ", use default settings.";
  }

  server->SetConfig(config);
}
