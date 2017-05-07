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

#include <common/threads/thread.h>
#include <common/threads/types.h>

#include <common/libev/io_loop_observer.h>
#include <common/libev/tcp/tcp_server.h>

#include "inner/inner_server_command_seq_parser.h"

#include "redis/redis_helpers.h"

#include "infos.h"

namespace fasto {
namespace fastotv {
namespace server {
class ServerHost;
namespace inner {

class InnerTcpClient;
class InnerSubHandler;

class InnerTcpHandlerHost : public fasto::fastotv::inner::InnerServerCommandSeqParser,
                            public common::libev::IoLoopObserver {
 public:
  enum {
    ping_timeout_clients = 60  // sec
  };

  explicit InnerTcpHandlerHost(ServerHost* parent);

  virtual void PreLooped(common::libev::IoLoop* server) override;

  virtual void Accepted(common::libev::IoClient* client) override;
  virtual void Moved(common::libev::IoClient* client) override;
  virtual void Closed(common::libev::IoClient* client) override;

  virtual void DataReceived(common::libev::IoClient* client) override;
  virtual void DataReadyToWrite(common::libev::IoClient* client) override;
  virtual void PostLooped(common::libev::IoLoop* server) override;
  virtual void TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) override;

  virtual ~InnerTcpHandlerHost();

  void SetStorageConfig(const redis_sub_configuration_t& config);

  bool PublishToChannelOut(const std::string& msg);
  inner::InnerTcpClient* FindInnerConnectionByID(const std::string& login) const;

 private:
  void PublishStateToChannel(user_id_t uid, bool connected);

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

  // inner handlers
  common::Error HandleInnerSuccsessResponceCommand(fastotv::inner::InnerClient* connection,
                                                   cmd_seq_t id,
                                                   int argc,
                                                   char* argv[]) WARN_UNUSED_RESULT;
  common::Error HandleInnerFailedResponceCommand(fastotv::inner::InnerClient* connection,
                                                 cmd_seq_t id,
                                                 int argc,
                                                 char* argv[]) WARN_UNUSED_RESULT;

  ServerHost* const parent_;

  RedisSub* sub_commands_in_;
  InnerSubHandler* handler_;
  std::shared_ptr<common::threads::Thread<void> > redis_subscribe_command_in_thread_;
  common::libev::timer_id_t ping_client_id_timer_;
};

}  // namespace inner
}  // namespace server
}  // namespace fastotv
}  // namespace fasto
