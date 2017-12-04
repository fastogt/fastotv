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

#include "client/inputs/lirc_input_client.h"

#include <lirc/lirc_client.h>  // for lirc_code2char, lirc_deinit

#include <common/file_system/string_path_utils.h>
#include <common/file_system/file_system.h>
#include <common/sprintf.h>
#include <common/string_util.h>  // for strdup
#include <common/utils.h>        // for freeifnotnull

namespace fastotv {
namespace client {
namespace inputs {

common::Error LircInit(int* fd, struct lirc_config** cfg) {
  if (!fd || !cfg) {
    return common::make_error_inval();
  }

  char* copy = common::strdup(PROJECT_NAME_LOWERCASE);  // copy for removing warning
  int lfd = lirc_init(copy, 1);
  common::utils::freeifnotnull(copy);
  if (lfd == -1) {
    return common::make_error("Lirc init failed!");
  }

  common::ErrnoError err = common::file_system::set_blocking_descriptor(lfd, false);
  if (err) {
    return common::ErrorValue(err->GetDescription());
  }

  lirc_config* lcfg = NULL;
  const std::string absolute_source_dir = common::file_system::absolute_path_from_relative(RELATIVE_SOURCE_DIR);
  const std::string lirc_config_path = common::file_system::make_path(absolute_source_dir, LIRCRC_CONFIG_PATH_RELATIVE);
  if (lirc_config_path.empty()) {
    return common::make_error("Lirc invalid config path!");
  }

  const char* lirc_config_path_ptr = lirc_config_path.c_str();
  char* lirc_config_copy_ptr = common::strdup(lirc_config_path_ptr);  // copy for removing warning
  int res = lirc_readconfig(lirc_config_copy_ptr, &lcfg, NULL);
  common::utils::freeifnotnull(lirc_config_copy_ptr);
  if (res == -1) {
    LircDeinit(lfd, NULL);
    std::string msg_error = common::MemSPrintf("Could not read LIRC config file: %s", lirc_config_path);
    return common::make_error(msg_error);
  }

  *fd = lfd;
  *cfg = lcfg;
  return common::Error();
}

common::Error LircDeinit(int fd, struct lirc_config** cfg) {
  if (fd == -1) {
    return common::Error();
  }

  if (lirc_deinit() == -1) {
    return common::make_error("Lirc deinit failed!");
  }

  if (cfg) {
    *cfg = NULL;
  }
  return common::Error();
}

LircInputClient::LircInputClient(common::libev::IoLoop* server, int fd, struct lirc_config* cfg)
    : base_class(server, fd), cfg_(cfg) {}

common::Error LircInputClient::ReadWithCallback(read_callback_t cb) {
  char* code = NULL;
  int ret;
  while ((ret = lirc_nextcode(&code)) == 0 && code != NULL) {
    char* c = NULL;
    while ((ret = lirc_code2char(cfg_, code, &c)) == 0 && c != NULL) {
      if (cb) {
        cb(c);
      }
    }
    free(code);
    if (ret == -1) {
      break;
    }
  }

  return common::Error();
}

common::Error LircInputClient::CloseImpl() {
  return LircDeinit(GetFd(), &cfg_);
}

}  // namespace inputs
}  // namespace client
}  // namespace fastotv
