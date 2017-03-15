/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of SiteOnYourDevice.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>

#include <common/patterns/crtp_pattern.h>
#include <common/net/socket_tcp.h>

#include "network/event_loop.h"

namespace fasto {
namespace fastotv {
namespace tcp {

class ITcpLoop;

class TcpClient : common::IMetaClassInfo {
 public:
  typedef int flags_t;
  friend class ITcpLoop;
  TcpClient(ITcpLoop* server, const common::net::socket_info& info, flags_t flags = EV_READ);
  virtual ~TcpClient();

  ITcpLoop* server() const;

  common::net::socket_info info() const;
  int fd() const;

  virtual common::Error write(const char* data, size_t size, ssize_t* nwrite) WARN_UNUSED_RESULT;
  virtual common::Error read(char* out, size_t max_size, ssize_t* nread) WARN_UNUSED_RESULT;

  void close();

  void setName(const std::string& name);
  std::string name() const;

  flags_t flags() const;
  void setFlags(flags_t flags);

  common::patterns::id_counter<TcpClient>::type_t id() const;
  virtual const char* ClassName() const override;
  std::string formatedName() const;

 private:
  ITcpLoop* server_;
  ev_io* read_write_io_;
  int flags_;

  common::net::SocketHolder sock_;
  std::string name_;
  const common::patterns::id_counter<TcpClient> id_;
};

}  // namespace tcp
}  // namespace fastotv
}  // namespace fasto
