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

#include "network/tcp/tcp_client.h"

#include <inttypes.h>

#include <string>

#include <common/logger.h>

#include "network/tcp/tcp_server.h"

namespace fasto {
namespace fastotv {
namespace tcp {

TcpClient::TcpClient(ITcpLoop* server, const common::net::socket_info& info, flags_t flags)
    : server_(server),
      read_write_io_((struct ev_io*)calloc(1, sizeof(struct ev_io))),
      flags_(flags),
      sock_(info),
      name_(),
      id_() {
  read_write_io_->data = this;
}

common::net::socket_info TcpClient::info() const {
  return sock_.info();
}

int TcpClient::fd() const {
  return sock_.fd();
}

common::Error TcpClient::write(const char* data, uint16_t size, ssize_t* nwrite) {
  return sock_.write(data, size, nwrite);
}

common::Error TcpClient::read(char* out, uint16_t max_size, ssize_t* nread) {
  return sock_.read(out, max_size, nread);
}

TcpClient::~TcpClient() {
  free(read_write_io_);
  read_write_io_ = NULL;
}

ITcpLoop* TcpClient::server() const {
  return server_;
}

void TcpClient::close() {
  if (server_) {
    server_->closeClient(this);
  }

  common::Error err = sock_.close();
  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
  }
}

void TcpClient::setName(const std::string& name) {
  name_ = name;
}

std::string TcpClient::name() const {
  return name_;
}

TcpClient::flags_t TcpClient::flags() const {
  return flags_;
}

void TcpClient::setFlags(flags_t flags) {
  flags_ = flags;
  server_->changeFlags(this);
}

common::patterns::id_counter<TcpClient>::type_t TcpClient::id() const {
  return id_.id();
}

const char* TcpClient::ClassName() const {
  return "TcpClient";
}

std::string TcpClient::formatedName() const {
  return common::MemSPrintf("[%s][%s(%" PRIuMAX ")]", name(), ClassName(), id());
}

}  // namespace tcp
}  // namespace fastotv
}  // namespace fasto
