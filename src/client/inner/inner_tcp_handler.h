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

#include <common/error.h>                   // for Error
#include <common/libev/io_loop_observer.h>  // for IoLoopObserver
#include <common/libev/types.h>             // for timer_id_t
#include <common/macros.h>                  // for WARN_UNUSED_RESULT
#include <common/net/types.h>               // for HostAndPort

#include "auth_info.h"  // for AuthInfo

#include "client/types.h"         // for BandwidthHostType
#include "client_server_types.h"  // for bandwidth_t

#include "commands/commands.h"  // for cmd_seq_t

#include "inner/inner_server_command_seq_parser.h"  // for InnerServerComman...

#include "third-party/json-c/json-c/json_object.h"  // for json_object

namespace common {
namespace libev {
class IoClient;
}
}  // namespace common
namespace common {
namespace libev {
class IoLoop;
}
}  // namespace common

namespace fasto {
namespace fastotv {
namespace inner {
class InnerClient;
}
}  // namespace fastotv
}  // namespace fasto

namespace fasto {
namespace fastotv {
namespace client {
namespace bandwidth {
class TcpBandwidthClient;
}
namespace inner {

struct StartConfig {
  common::net::HostAndPort inner_host;
  AuthInfo ainf;
};

class InnerTcpHandler : public fasto::fastotv::inner::InnerServerCommandSeqParser,
                        public common::libev::IoLoopObserver {
 public:
  enum {
    ping_timeout_server = 30  // sec
  };

  explicit InnerTcpHandler(const StartConfig& config);
  virtual ~InnerTcpHandler();

  void RequestServerInfo();                     // should be execute in network thread
  void RequestChannels();                       // should be execute in network thread
  void Connect(common::libev::IoLoop* server);  // should be execute in network thread
  void DisConnect(common::Error err);           // should be execute in network thread

  virtual void PreLooped(common::libev::IoLoop* server) override;
  virtual void Accepted(common::libev::IoClient* client) override;
  virtual void Moved(common::libev::IoLoop* server, common::libev::IoClient* client) override;
  virtual void Closed(common::libev::IoClient* client) override;
  virtual void DataReceived(common::libev::IoClient* client) override;
  virtual void DataReadyToWrite(common::libev::IoClient* client) override;
  virtual void PostLooped(common::libev::IoLoop* server) override;
  virtual void TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) override;

 private:
  common::Error CreateAndConnectTcpBandwidthClient(common::libev::IoLoop* server,
                                                   const common::net::HostAndPort& host,
                                                   BandwidthHostType hs,
                                                   bandwidth::TcpBandwidthClient** out_band) WARN_UNUSED_RESULT;

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

  // inner handlers
  common::Error HandleInnerSuccsessResponceCommand(fastotv::inner::InnerClient* connection,
                                                   cmd_seq_t id,
                                                   int argc,
                                                   char* argv[]) WARN_UNUSED_RESULT;
  common::Error HandleInnerFailedResponceCommand(fastotv::inner::InnerClient* connection,
                                                 cmd_seq_t id,
                                                 int argc,
                                                 char* argv[]) WARN_UNUSED_RESULT;

  common::Error ParserResponceResponceCommand(int argc, char* argv[], json_object** out) WARN_UNUSED_RESULT;

  fasto::fastotv::inner::InnerClient* inner_connection_;
  std::vector<bandwidth::TcpBandwidthClient*> bandwidth_requests_;
  common::libev::timer_id_t ping_server_id_timer_;

  const StartConfig config_;

  bandwidth_t current_bandwidth_;
};

}  // namespace inner
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
