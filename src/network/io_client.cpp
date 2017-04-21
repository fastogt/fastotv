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

#include "network/io_client.h"

#include <inttypes.h>

#include <string>

#include <common/logger.h>

#include "network/tcp/tcp_server.h"

namespace fasto {
namespace fastotv {
namespace network {

IoClient::IoClient(IoLoop* server, flags_t flags)
    : server_(server),
      read_write_io_(static_cast<struct ev_io*>(calloc(1, sizeof(struct ev_io)))),
      flags_(flags),
      name_(),
      id_() {
  read_write_io_->data = this;
}

IoClient::~IoClient() {
  free(read_write_io_);
  read_write_io_ = NULL;
}

void IoClient::Close() {
  if (server_) {
    server_->CloseClient(this);
  }
  CloseImpl();
}

IoLoop* IoClient::Server() const {
  return server_;
}

void IoClient::SetName(const std::string& name) {
  name_ = name;
}

std::string IoClient::Name() const {
  return name_;
}

IoClient::flags_t IoClient::Flags() const {
  return flags_;
}

void IoClient::SetFlags(flags_t flags) {
  flags_ = flags;
  server_->ChangeFlags(this);
}

common::patterns::id_counter<IoClient>::type_t IoClient::Id() const {
  return id_.id();
}

const char* IoClient::ClassName() const {
  return "IoClient";
}

std::string IoClient::FormatedName() const {
  return common::MemSPrintf("[%s][%s(%" PRIuMAX ")]", Name(), ClassName(), Id());
}
}
}  // namespace fastotv
}  // namespace fasto
