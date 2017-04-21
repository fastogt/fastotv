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

#include "network/io_client.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
namespace inputs {

common::Error LircInit(int* fd) WARN_UNUSED_RESULT;
common::Error LircDeinit(int fd) WARN_UNUSED_RESULT;

class LircInputClient : public network::IoClient {
 public:
  LircInputClient(network::IoLoop* server, int fd);

  virtual int Fd() const;

 protected:
  virtual void CloseImpl() override;

 private:
  virtual common::Error Write(const char* data,
                              size_t size,
                              size_t* nwrite) final WARN_UNUSED_RESULT;
  virtual common::Error Read(char* out, size_t max_size, size_t* nread) final WARN_UNUSED_RESULT;

  common::net::SocketHolder sock_;
};
}
}
}
}
}
