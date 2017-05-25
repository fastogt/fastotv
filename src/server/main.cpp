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

#include <unistd.h>

#include <string>
#include <iostream>

#include <common/logger.h>
#include <common/utils.h>
#include <common/file_system.h>
#include <common/convert2string.h>

#include "server_host.h"

const char* config_path = SERVER_CONFIG_FILE_PATH;

int main(int argc, char* argv[]) {
  int opt;
  while ((opt = getopt(argc, argv, "cd:")) != -1) {
    switch (opt) {
      case 'c':
        config_path = argv[optind];
        break;
      default: /* '?' */
        fprintf(stderr, "Usage: %s [-c] config path\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }

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

  fasto::fastotv::server::Config config;
  fasto::fastotv::server::load_config_file(config_path, &config);
  fasto::fastotv::server::ServerHost server(config);
  return server.Exec();
}
