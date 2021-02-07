/*  Copyright (C) 2014-2020 FastoGT. All right reserved.

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

#include <string>
#include <vector>

#include <common/libev/io_loop_observer.h>  // for IoLoopObserver
#include <common/net/types.h>               // for HostAndPort

#include <fastotv/commands_info/auth_info.h>  // for AuthInfo
#include <fastotv/protocol/types.h>
#include <fastotv/types.h>  // for bandwidth_t

namespace fastotv {
namespace client {
class Client;
namespace bandwidth {
class TcpBandwidthClient;
}
namespace inner {

class InnerTcpHandler : public common::libev::IoLoopObserver {
 public:
  enum {
    ping_timeout_server = 30  // sec
  };

  explicit InnerTcpHandler(const common::net::HostAndPort& server_host, const commands_info::AuthInfo& auth_info);
  ~InnerTcpHandler() override;

  void ActivateRequest();                          // should be execute in network thread
  void RequestServerInfo();                        // should be execute in network thread
  void RequestChannels();                          // should be execute in network thread
  void RequesRuntimeChannelInfo(stream_id_t sid);  // should be execute in network thread
  void Connect(common::libev::IoLoop* server);     // should be execute in network thread
  void DisConnect(common::Error err);              // should be execute in network thread

  void PreLooped(common::libev::IoLoop* server) override;
  void Accepted(common::libev::IoClient* client) override;
  void Moved(common::libev::IoLoop* server, common::libev::IoClient* client) override;
  void Closed(common::libev::IoClient* client) override;
  void DataReceived(common::libev::IoClient* client) override;
  void DataReadyToWrite(common::libev::IoClient* client) override;
  void PostLooped(common::libev::IoLoop* server) override;
  void TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) override;
  void Accepted(common::libev::IoChild* child) override;
  void Moved(common::libev::IoLoop* server, common::libev::IoChild* child) override;
  void ChildStatusChanged(common::libev::IoChild* child, int status, int signal) override;

 protected:
  common::ErrnoError HandleInnerDataReceived(Client* client, const std::string& input_command);
  common::ErrnoError HandleRequestCommand(Client* client, const protocol::request_t* req);
  common::ErrnoError HandleResponceCommand(Client* client, const protocol::response_t* resp);

 private:
  common::ErrnoError HandleRequestServerPing(Client* client, const protocol::request_t* req);
  common::ErrnoError HandleRequestServerClientInfo(Client* client, const protocol::request_t* req);
  common::ErrnoError HandleRequestServerTextNotification(Client* client, const protocol::request_t* req);
  common::ErrnoError HandleRequestServerShutdownNotification(Client* client, const protocol::request_t* req);

  common::ErrnoError HandleResponceClientActivateDevice(Client* client, const protocol::response_t* resp);
  common::ErrnoError HandleResponceClientLogin(Client* client, const protocol::response_t* resp);
  common::ErrnoError HandleResponceClientPing(Client* client, const protocol::response_t* resp);
  common::ErrnoError HandleResponceClientGetServerInfo(Client* client, const protocol::response_t* resp);
  common::ErrnoError HandleResponceClientGetChannels(Client* client, const protocol::response_t* resp);
  common::ErrnoError HandleResponceClientGetruntimeChannelInfo(Client* client, const protocol::response_t* resp);

  Client* inner_connection_;
  common::libev::timer_id_t ping_server_id_timer_;

  const common::net::HostAndPort server_host_;
  const commands_info::AuthInfo auth_info_;
};

}  // namespace inner
}  // namespace client
}  // namespace fastotv
