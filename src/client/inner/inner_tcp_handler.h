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

#include "infos.h"

#include "inner/inner_server_command_seq_parser.h"

#include <common/libev/tcp/tcp_server.h>

namespace fasto {
namespace fastotv {
namespace client {
namespace inner {

class InnerTcpHandler : public fasto::fastotv::inner::InnerServerCommandSeqParser,
                        public common::libev::IoLoopObserver {
 public:
  enum {
    ping_timeout_server = 30  // sec
  };

  explicit InnerTcpHandler(const common::net::HostAndPort& innerHost, const AuthInfo& ainf);
  ~InnerTcpHandler();

  void RequestChannels();                 // should be execute in network thread
  void Connect(common::libev::IoLoop* server);  // should be execute in network thread
  void DisConnect(common::Error err);     // should be execute in network thread

  virtual void PreLooped(common::libev::IoLoop* server) override;
  virtual void Accepted(common::libev::IoClient* client) override;
  virtual void Moved(common::libev::IoClient* client) override;
  virtual void Closed(common::libev::IoClient* client) override;
  virtual void DataReceived(common::libev::IoClient* client) override;
  virtual void DataReadyToWrite(common::libev::IoClient* client) override;
  virtual void PostLooped(common::libev::IoLoop* server) override;
  virtual void TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) override;

 private:
  virtual void HandleInnerRequestCommand(fasto::fastotv::inner::InnerClient* connection,
                                         cmd_seq_t id,
                                         int argc,
                                         char* argv[]) override;
  virtual void HandleInnerResponceCommand(fasto::fastotv::inner::InnerClient* connection,
                                          cmd_seq_t id,
                                          int argc,
                                          char* argv[]) override;
  virtual void HandleInnerApproveCommand(fasto::fastotv::inner::InnerClient* connection,
                                         cmd_seq_t id,
                                         int argc,
                                         char* argv[]) override;

  fasto::fastotv::inner::InnerClient* inner_connection_;
  common::libev::timer_id_t ping_server_id_timer_;

  const common::net::HostAndPort innerHost_;
  const AuthInfo ainf_;
};

}  // namespace inner
}
}  // namespace fastotv
}  // namespace fasto
