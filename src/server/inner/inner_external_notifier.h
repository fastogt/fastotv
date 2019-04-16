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

#include <string>  // for string

#include "commands/commands.h"
#include "protocol/types.h"

#include "server/redis/redis_pub_sub_handler.h"
#include "server/user_request_info.h"

namespace fastotv {
namespace server {
namespace inner {

class InnerTcpHandlerHost;

class InnerSubHandler : public redis::RedisSubHandler {
 public:
  explicit InnerSubHandler(InnerTcpHandlerHost* parent);
  virtual ~InnerSubHandler();

 protected:
  void HandleMessage(const std::string& channel, const std::string& msg) override;

 private:
  common::ErrnoError HandleRequest(const UserRequestInfo& request);

  void PublishResponse(const UserRpcInfo& uinf, const protocol::response_t* resp);

  InnerTcpHandlerHost* parent_;
};

}  // namespace inner
}  // namespace server
}  // namespace fastotv
