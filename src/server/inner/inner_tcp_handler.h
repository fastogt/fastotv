/*  Copyright (C) 2014-2019 FastoGT. All right reserved.

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

#include <memory>  // for shared_ptr
#include <string>  // for string
#include <vector>

#include <common/error.h>                   // for Error
#include <common/libev/io_loop_observer.h>  // for IoLoopObserver
#include <common/libev/types.h>             // for timer_id_t
#include <common/macros.h>                  // for WARN_UNUSED_RESULT

#include "commands/commands.h"
#include "inner/inner_server_command_seq_parser.h"  // for InnerServerComman...

#include "server/config.h"  // for Config
#include "server/rpc/user_rpc_info.h"

#include "commands_info/chat_message.h"

namespace common {
namespace libev {
class IoClient;
class IoLoop;
}  // namespace libev
namespace threads {
template <typename RT>
class Thread;
}
}  // namespace common

namespace fastotv {
namespace inner {
class InnerClient;
}
namespace server {
class ServerHost;
namespace redis {
class RedisPubSub;
}
namespace inner {

class InnerSubHandler;
class InnerTcpClient;

class InnerTcpHandlerHost : public fastotv::inner::InnerServerCommandSeqParser, public common::libev::IoLoopObserver {
 public:
  enum {
    ping_timeout_clients = 60,  // sec
    reread_cache_timeout = 150
  };

  explicit InnerTcpHandlerHost(ServerHost* parent, const Config& config);

  void PreLooped(common::libev::IoLoop* server) override;

  void Accepted(common::libev::IoClient* client) override;
  void Moved(common::libev::IoLoop* server, common::libev::IoClient* client) override;
  void Closed(common::libev::IoClient* client) override;

  void DataReceived(common::libev::IoClient* client) override;
  void DataReadyToWrite(common::libev::IoClient* client) override;
  void PostLooped(common::libev::IoLoop* server) override;
  void TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) override;
#if LIBEV_CHILD_ENABLE
  void Accepted(common::libev::IoChild* child) override;
  void Moved(common::libev::IoLoop* server, common::libev::IoChild* child) override;
  void ChildStatusChanged(common::libev::IoChild* client, int status) override;
#endif

  virtual ~InnerTcpHandlerHost();

  common::Error PublishToChannelOut(const std::string& msg);
  inner::InnerTcpClient* FindInnerConnectionByUser(const rpc::UserRpcInfo& user) const;

 private:
  void UpdateCache();
  void PublishUserStateInfo(const rpc::UserRpcInfo& user, bool connected);

  common::ErrnoError HandleRequestCommand(fastotv::inner::InnerClient* client, protocol::request_t* req) override;
  common::ErrnoError HandleResponceCommand(fastotv::inner::InnerClient* client, protocol::response_t* resp) override;

  common::ErrnoError HandleRequestClientActivate(InnerTcpClient* client, protocol::request_t* req);
  common::ErrnoError HandleRequestClientPing(InnerTcpClient* client, protocol::request_t* req);
  common::ErrnoError HandleRequestClientGetServerInfo(InnerTcpClient* client, protocol::request_t* req);
  common::ErrnoError HandleRequestClientGetChannels(InnerTcpClient* client, protocol::request_t* req);
  common::ErrnoError HandleRequestClientGetRuntimeChannelInfo(InnerTcpClient* client, protocol::request_t* req);
  common::ErrnoError HandleRequestClientSendChatMessage(InnerTcpClient* client, protocol::request_t* req);

  common::ErrnoError HandleResponceServerPing(InnerTcpClient* client, protocol::response_t* resp);
  common::ErrnoError HandleResponceServerGetClientInfo(InnerTcpClient* client, protocol::response_t* resp);
  common::ErrnoError HandleResponceServerSendChatMessage(InnerTcpClient* client, protocol::response_t* resp);

  void SendEnterChatMessage(common::libev::IoLoop* server, stream_id sid, login_t login);
  void SendLeaveChatMessage(common::libev::IoLoop* server, stream_id sid, login_t login);
  void BrodcastChatMessage(common::libev::IoLoop* server, const ChatMessage& msg);
  size_t GetOnlineUserByStreamID(common::libev::IoLoop* server, stream_id sid) const;

  ServerHost* const parent_;

  redis::RedisPubSub* sub_commands_in_;
  InnerSubHandler* handler_;
  std::shared_ptr<common::threads::Thread<void>> redis_subscribe_command_in_thread_;
  common::libev::timer_id_t ping_client_id_timer_;
  common::libev::timer_id_t reread_cache_id_timer_;
  const Config config_;

  mutable std::vector<stream_id> chat_channels_;
};

}  // namespace inner
}  // namespace server
}  // namespace fastotv
