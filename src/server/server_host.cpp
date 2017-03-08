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

#include "server/server_host.h"

#include <unistd.h>

#include <string>

#include <common/threads/thread_manager.h>
#include <common/logger.h>

#define BUF_SIZE 4096

namespace fasto {
namespace fastotv {
namespace server {

ServerHandlerHost::ServerHandlerHost(server_t* parent) {}

void ServerHandlerHost::accepted(tcp::TcpClient* client) {}

void ServerHandlerHost::closed(tcp::TcpClient* client) {}

void ServerHandlerHost::dataReceived(tcp::TcpClient* client) {}

ServerHandlerHost::~ServerHandlerHost() {}

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
