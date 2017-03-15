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

#include "client/tv_config.h"

#include "network/tcp/tcp_server.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace inner {

class InnerTcpServer : public tcp::ITcpLoop {
 public:
  explicit InnerTcpServer(tcp::ITcpLoopObserver* observer);
  virtual const char* ClassName() const override;

 protected:
  tcp::TcpClient* CreateClient(const common::net::socket_info& info) override;
};

}  // namespace inner
}
}  // namespace siteonyourdevice
}  // namespace fasto
