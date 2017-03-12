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

#include "infos.h"

#include "inner/inner_server_command_seq_parser.h"

#include "network/tcp/tcp_server.h"

#include "tv_config.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace inner {

class InnerTcpHandler : public fasto::fastotv::inner::InnerServerCommandSeqParser,
                        public tcp::ITcpLoopObserver {
 public:
  enum {
    ping_timeout_server = 30  // sec
  };

  explicit InnerTcpHandler(const common::net::HostAndPort& innerHost, const TvConfig& config);
  ~InnerTcpHandler();

  AuthInfo authInfo() const;
  void setConfig(const TvConfig& config);

  void RequestChannels();  // should be execute in network thread

  virtual void preLooped(tcp::ITcpLoop* server) override;
  virtual void accepted(tcp::TcpClient* client) override;
  virtual void moved(tcp::TcpClient* client) override;
  virtual void closed(tcp::TcpClient* client) override;
  virtual void dataReceived(tcp::TcpClient* client) override;
  virtual void dataReadyToWrite(tcp::TcpClient* client) override;
  virtual void postLooped(tcp::ITcpLoop* server) override;
  virtual void timerEmited(tcp::ITcpLoop* server, timer_id_t id) override;

 private:
  virtual void handleInnerRequestCommand(fasto::fastotv::inner::InnerClient* connection,
                                         cmd_seq_t id,
                                         int argc,
                                         char* argv[]) override;
  virtual void handleInnerResponceCommand(fasto::fastotv::inner::InnerClient* connection,
                                          cmd_seq_t id,
                                          int argc,
                                          char* argv[]) override;
  virtual void handleInnerApproveCommand(fasto::fastotv::inner::InnerClient* connection,
                                         cmd_seq_t id,
                                         int argc,
                                         char* argv[]) override;

  TvConfig config_;
  fasto::fastotv::inner::InnerClient* inner_connection_;
  timer_id_t ping_server_id_timer_;

  const common::net::HostAndPort innerHost_;
};

}  // namespace inner
}
}  // namespace fastotv
}  // namespace fasto
