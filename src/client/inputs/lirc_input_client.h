/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

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

#include <functional>

#include <common/libev/descriptor_client.h>

struct lirc_config;

namespace common {
namespace libev {
class IoLoop;
}
}  // namespace common

namespace fastotv {
namespace client {
namespace inputs {

common::Error LircInit(int* fd, struct lirc_config** cfg) WARN_UNUSED_RESULT;
common::Error LircDeinit(int fd, struct lirc_config** cfg) WARN_UNUSED_RESULT;

class LircInputClient : public common::libev::DescriptorClient {
 public:
  typedef common::libev::DescriptorClient base_class;
  typedef std::function<void(const std::string&)> read_callback_t;
  LircInputClient(common::libev::IoLoop* server, int fd, struct lirc_config* cfg);

  common::Error ReadWithCallback(read_callback_t cb) WARN_UNUSED_RESULT;

 private:
  virtual common::Error CloseImpl() override;

  using base_class::Write;
  using base_class::Read;

  struct lirc_config* cfg_;
};
}  // namespace inputs
}  // namespace client
}  // namespace fastotv
