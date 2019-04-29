/*  Copyright (C) 2014-2019 FastoGT. All right reserved.

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

#include <stdio.h>   // for fprintf, stderr
#include <stdlib.h>  // for exit, EXIT_FAILURE
#include <unistd.h>  // for getopt, optind

#include <common/log_levels.h>  // for LOG_LEVEL, LOG_LEVEL::L_DEBUG

#include "server/config.h"  // for Config
#include "server_host.h"    // for ServerHost

namespace {
const char* kConfigPath = SERVER_CONFIG_FILE_PATH;
}

int main(int argc, char* argv[]) {
  int opt;
  while ((opt = getopt(argc, argv, "cd:")) != -1) {
    switch (opt) {
      case 'c':
        kConfigPath = argv[optind];
        break;
      default: /* '?' */
        fprintf(stderr, "Usage: %s [-c] config path\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

#if defined(NDEBUG)
  common::logging::LOG_LEVEL level = common::logging::LOG_LEVEL_INFO;
#else
  common::logging::LOG_LEVEL level = common::logging::LOG_LEVEL_DEBUG;
#endif
#if defined(LOG_TO_FILE)
  std::string log_path = common::file_system::prepare_path("/var/log/" PROJECT_NAME_LOWERCASE ".log");
  INIT_LOGGER(PROJECT_NAME_TITLE, log_path, level);
#else
  INIT_LOGGER(PROJECT_NAME_TITLE, level);
#endif

  fastotv::server::Config config;
  common::Error err = fastotv::server::load_config_file(kConfigPath, &config);
  if (err) {
    return EXIT_FAILURE;
  }
  fastotv::server::ServerHost server(config);
  return server.Exec();
}
