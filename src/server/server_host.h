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

#include "network/tcp/tcp_server.h"

#include <common/threads/thread.h>

namespace fasto {
namespace fastotv {
namespace server {

class ServerHost;

class ServerHandlerHost : public tcp::ITcpLoopObserver {
 public:
  typedef tcp::TcpServer server_t;
  typedef tcp::TcpClient client_t;

  ServerHandlerHost(server_t* parent);
  virtual void accepted(tcp::TcpClient* client) override;
  virtual void closed(tcp::TcpClient* client) override;
  virtual void dataReceived(tcp::TcpClient* client) override;

  virtual ~ServerHandlerHost();

 private:
  ServerHost* const parent_;
};

class ServerHost : public tcp::TcpServer {

};

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
