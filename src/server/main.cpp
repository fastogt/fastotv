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

#include <common/logger.h>
#include <common/utils.h>
#include <common/file_system.h>
#include <common/convert2string.h>

#include "server_host.h"
#include "server_config.h"

#define HOST_PATH "/"

const char* config_path = SERVER_CONFIG_FILE_PATH;

#ifdef OS_POSIX
sig_atomic_t is_stop = 0;
#endif

namespace {
fasto::fastotv::server::Config config;
fasto::fastotv::server::ServerHost* server = NULL;
}

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
  fasto::fastotv::server::load_config_file(config_path, &config);
  server->SetConfig(config);
}
