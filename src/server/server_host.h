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

#include <common/threads/thread.h>

#include "network/tcp/tcp_server.h"

#include "redis/redis_helpers.h"

namespace fasto {
namespace fastotv {
namespace server {

struct Settings {
  redis_sub_configuration_t redis;
};

struct Config {
  Settings server;
};

class ServerHandlerHost : public tcp::ITcpLoopObserver {
 public:
  typedef tcp::TcpServer server_t;
  typedef tcp::TcpClient client_t;

  ServerHandlerHost();
  virtual void preLooped(tcp::ITcpLoop* server) override;

  virtual void accepted(tcp::TcpClient* client) override;
  virtual void moved(tcp::TcpClient* client) override;
  virtual void closed(tcp::TcpClient* client) override;
  virtual void timerEmited(tcp::ITcpLoop* server, timer_id_t id) override;

  virtual void dataReceived(tcp::TcpClient* client) override;
  virtual void dataReadyToWrite(tcp::TcpClient* client) override;

  virtual void postLooped(tcp::ITcpLoop *server) override;

  virtual ~ServerHandlerHost();
};

class ServerHost {
 public:
  explicit ServerHost(const common::net::HostAndPort& host);
  ~ServerHost();

  void stop();
  int exec();
  void setConfig(const Config& conf);

 private:
  bool stop_;

  tcp::TcpServer* server_;
  ServerHandlerHost* handler_;

  RedisStorage rstorage_;
};

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
