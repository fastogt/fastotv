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

#include <string>

#include <common/patterns/crtp_pattern.h>
#include <common/net/socket_tcp.h>

#include "network/event_loop.h"

namespace fasto {
namespace fastotv {
namespace network {
namespace tcp {

class ITcpLoop;

class TcpClient : common::IMetaClassInfo {
 public:
  typedef int flags_t;
  friend class ITcpLoop;
  TcpClient(ITcpLoop* server, const common::net::socket_info& info, flags_t flags = EV_READ);
  virtual ~TcpClient();

  ITcpLoop* Server() const;

  common::net::socket_info Info() const;
  int Fd() const;

  void Close();

  void SetName(const std::string& name);
  std::string Name() const;

  flags_t Flags() const;
  void SetFlags(flags_t flags);

  common::patterns::id_counter<TcpClient>::type_t Id() const;
  virtual const char* ClassName() const override;
  std::string FormatedName() const;

 protected:
  virtual common::Error Write(const char* data, size_t size, size_t* nwrite) WARN_UNUSED_RESULT;
  virtual common::Error Read(char* out, size_t max_size, ssize_t* nread) WARN_UNUSED_RESULT;

 private:
  ITcpLoop* server_;
  ev_io* read_write_io_;
  int flags_;

  common::net::SocketHolder sock_;
  std::string name_;
  const common::patterns::id_counter<TcpClient> id_;
};

}  // namespace tcp
}
}  // namespace fastotv
}  // namespace fasto
