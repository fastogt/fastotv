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

#include <common/threads/thread.h>
#include <common/threads/types.h>

#include "inner/inner_server_command_seq_parser.h"

#include "redis/redis_helpers.h"

#include "network/tcp/tcp_server.h"

#include "infos.h"

namespace fasto {
namespace fastotv {
namespace server {
class ServerHost;
namespace inner {

class InnerTcpClient;
class InnerSubHandler;

class InnerTcpHandlerHost : public fasto::fastotv::inner::InnerServerCommandSeqParser,
                            public tcp::ITcpLoopObserver {
 public:
  enum {
    ping_timeout_clients = 60  // sec
  };

  explicit InnerTcpHandlerHost(ServerHost* parent);

  virtual void preLooped(tcp::ITcpLoop* server) override;

  virtual void accepted(tcp::TcpClient* client) override;
  virtual void moved(tcp::TcpClient* client) override;
  virtual void closed(tcp::TcpClient* client) override;

  virtual void dataReceived(tcp::TcpClient* client) override;
  virtual void dataReadyToWrite(tcp::TcpClient* client) override;
  virtual void postLooped(tcp::ITcpLoop* server) override;
  virtual void timerEmited(tcp::ITcpLoop* server, timer_id_t id) override;

  virtual ~InnerTcpHandlerHost();

  void setStorageConfig(const redis_sub_configuration_t& config);

  bool PublishToChannelOut(const std::string& msg);
  inner::InnerTcpClient* FindInnerConnectionByLogin(const std::string& login) const;

 private:
  void PublishStateToChannel(const std::string& login, bool connected);

  virtual void HandleInnerRequestCommand(fastotv::inner::InnerClient* connection,
                                         cmd_seq_t id,
                                         int argc,
                                         char* argv[]) override;
  virtual void HandleInnerResponceCommand(fastotv::inner::InnerClient* connection,
                                          cmd_seq_t id,
                                          int argc,
                                          char* argv[]) override;
  virtual void HandleInnerApproveCommand(fastotv::inner::InnerClient* connection,
                                         cmd_seq_t id,
                                         int argc,
                                         char* argv[]) override;

  ServerHost* const parent_;

  RedisSub* sub_commands_in_;
  InnerSubHandler* handler_;
  std::shared_ptr<common::threads::Thread<void> > redis_subscribe_command_in_thread_;
  timer_id_t ping_client_id_timer_;
};

}  // namespace inner
}  // namespace server
}  // namespace fastotv
}  // namespace fasto
