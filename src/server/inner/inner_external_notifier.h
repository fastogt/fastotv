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

#include <string>  // for string

#include "commands/commands.h"

#include "server/redis/redis_pub_sub_handler.h"

namespace fastotv {
namespace server {
class ResponceInfo;
namespace inner {

class InnerTcpHandlerHost;

class InnerSubHandler : public redis::RedisSubHandler {
 public:
  explicit InnerSubHandler(InnerTcpHandlerHost* parent);
  virtual ~InnerSubHandler();

 protected:
  virtual void HandleMessage(const std::string& channel, const std::string& msg) override;

 private:
  void ProcessSubscribed(common::protocols::three_way_handshake::cmd_seq_t request_id, int argc, char* argv[]);

  void PublishResponce(const ResponceInfo& resp);

  InnerTcpHandlerHost* parent_;
};

}  // namespace inner
}  // namespace server
}  // namespace fastotv
