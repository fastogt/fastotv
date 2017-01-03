/*  Copyright (C) 2014-2017 FastoGT. All right reserved.

    This file is part of gpu_player.

    gpu_player is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    gpu_player is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with gpu_player.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <common/logger.h>
#include <common/file_system.h>

int main(int argc, char* argv[]) {
#if defined(LOG_TO_FILE)
  std::string log_path = common::file_system::prepare_path("~/" PROJECT_NAME_LOWERCASE ".log");
  INIT_LOGGER(PROJECT_NAME_TITLE, log_path);
#else
  INIT_LOGGER(PROJECT_NAME_TITLE);
#endif
#if defined(NDEBUG)
  SET_LOG_LEVEL(common::logging::L_INFO);
#else
  SET_LOG_LEVEL(common::logging::L_DEBUG);
#endif
  return 0;
}
