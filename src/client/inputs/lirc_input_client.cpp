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

#include <common/logger.h>

#include <lirc/lirc_client.h>

namespace fasto {
namespace fastotv {
namespace client {
namespace inputs {

common::Error LircInit(int* fd, struct lirc_config** cfg) {
  if (!fd || !cfg) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  int lfd = lirc_init(PROJECT_NAME_LOWERCASE, 1);
  if (lfd == -1) {
    return common::make_error_value("Lirc init failed!", common::Value::E_ERROR);
  }

  lirc_config* lcfg = NULL;
  if (lirc_readconfig(NULL, &lcfg, NULL) == -1) {
    LircDeinit(fd, NULL);
    return common::make_error_value("Could not read LIRC config file!", common::Value::E_ERROR);
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
    return common::make_error_value("Lirc deinit failed!", common::Value::E_ERROR);
  }

  if (cfg) {
    *cfg = NULL;
  }
  return common::Error();
}

LircInputClient::LircInputClient(network::IoLoop* server, int fd, struct lirc_config* cfg)
    : network::IoClient(server), sock_(fd), cfg_(cfg) {}

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

int LircInputClient::Fd() const {
  return sock_.fd();
}

common::Error LircInputClient::Write(const char* data, size_t size, size_t* nwrite) {
  return sock_.write(data, size, nwrite);
}

common::Error LircInputClient::Read(char* out, size_t max_size, size_t* nread) {
  if (!out || !nread) {
    return common::make_error_value("Invalid input argument(s)", common::ErrorValue::E_ERROR);
  }

  return sock_.read(out, max_size, nread);
}

void LircInputClient::CloseImpl() {
  common::Error err = LircDeinit(sock_.fd(), &cfg_);
  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
  }
}
}
}
}
}
