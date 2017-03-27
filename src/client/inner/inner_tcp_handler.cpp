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

#include "client/inner/inner_tcp_handler.h"

#include <string>

#include <common/error.h>
#include <common/logger.h>
#include <common/net/net.h>
#include <common/system_info/cpu_info.h>
#include <common/system_info/system_info.h>
#include <common/convert2string.h>
#include <common/application/application.h>

#include "third-party/json-c/json-c/json.h"

#include "network/tcp/tcp_server.h"

#include "inner/inner_client.h"

#include "client/commands.h"

#include "client/core/events/network_events.h"

#define STATUS_OS_FIELD "os"
#define STATUS_CPU_FIELD "cpu"
#define STATUS_RAM_TOTAL_FIELD "ram_total"
#define STATUS_RAM_FREE_FIELD "ram_free"

namespace fasto {
namespace fastotv {
namespace client {
namespace inner {

InnerTcpHandler::InnerTcpHandler(const common::net::HostAndPort& innerHost, const AuthInfo& ainf)
    : inner_connection_(nullptr),
      ping_server_id_timer_(INVALID_TIMER_ID),
      innerHost_(innerHost),
      ainf_(ainf) {}

InnerTcpHandler::~InnerTcpHandler() {
  delete inner_connection_;
  inner_connection_ = nullptr;
}

void InnerTcpHandler::PreLooped(network::tcp::ITcpLoop* server) {
  ping_server_id_timer_ = server->CreateTimer(ping_timeout_server, ping_timeout_server);
  CHECK(!inner_connection_);

  common::net::socket_info client_info;
  common::ErrnoError err =
      common::net::connect(innerHost_, common::net::ST_SOCK_STREAM, 0, &client_info);
  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
    auto ex_event = make_exception_event(new core::events::ClientConnectedEvent(this, ainf_), err);
    fApp->PostEvent(ex_event);
    return;
  }

  fasto::fastotv::inner::InnerClient* connection =
      new fasto::fastotv::inner::InnerClient(server, client_info);
  inner_connection_ = connection;
  server->RegisterClient(connection);
}

void InnerTcpHandler::Accepted(network::tcp::TcpClient* client) {
  UNUSED(client);
}

void InnerTcpHandler::Moved(network::tcp::TcpClient* client) {
  UNUSED(client);
}

void InnerTcpHandler::Closed(network::tcp::TcpClient* client) {
  if (client == inner_connection_) {
    fApp->PostEvent(new core::events::ClientDisconnectedEvent(this, ainf_));
    inner_connection_ = nullptr;
    return;
  }
}

void InnerTcpHandler::DataReceived(network::tcp::TcpClient* client) {
  std::string buff;
  fasto::fastotv::inner::InnerClient* iclient =
      static_cast<fasto::fastotv::inner::InnerClient*>(client);
  common::Error err = iclient->ReadCommand(&buff);
  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
    client->Close();
    delete client;
    return;
  }

  HandleInnerDataReceived(iclient, buff);
}

void InnerTcpHandler::DataReadyToWrite(network::tcp::TcpClient* client) {
  UNUSED(client);
}

void InnerTcpHandler::PostLooped(network::tcp::ITcpLoop* server) {
  UNUSED(server);
  if (inner_connection_) {
    fasto::fastotv::inner::InnerClient* connection = inner_connection_;
    connection->Close();
    delete connection;
  }
}

void InnerTcpHandler::TimerEmited(network::tcp::ITcpLoop* server, network::timer_id_t id) {
  UNUSED(server);
  if (id == ping_server_id_timer_ && inner_connection_) {
    const cmd_request_t ping_request = PingRequest(NextRequestID());
    size_t nwrite = 0;
    fasto::fastotv::inner::InnerClient* client = inner_connection_;
    common::Error err = client->Write(ping_request, &nwrite);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
      client->Close();
      delete client;
    }
  }
}

void InnerTcpHandler::RequestChannels() {
  if (inner_connection_) {
    const cmd_request_t channels_request = GetChannelsRequest(NextRequestID());
    size_t nwrite = 0;
    fasto::fastotv::inner::InnerClient* client = inner_connection_;
    common::Error err = client->Write(channels_request, &nwrite);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
      client->Close();
      delete client;
    }
  }
}

void InnerTcpHandler::HandleInnerRequestCommand(fasto::fastotv::inner::InnerClient* connection,
                                                cmd_seq_t id,
                                                int argc,
                                                char* argv[]) {
  UNUSED(argc);
  size_t nwrite = 0;
  char* command = argv[0];

  if (IS_EQUAL_COMMAND(command, SERVER_PING_COMMAND)) {
    const cmd_responce_t pong = PingResponceSuccsess(id);
    common::Error err = connection->Write(pong, &nwrite);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
  } else if (IS_EQUAL_COMMAND(command, SERVER_WHO_ARE_YOU_COMMAND)) {
    json_object* jain = AuthInfo::MakeJobject(ainf_);
    std::string auth_str = json_object_get_string(jain);
    json_object_put(jain);
    std::string enc_auth = common::HexEncode(auth_str, false);
    cmd_responce_t iAm = WhoAreYouResponceSuccsess(id, enc_auth);
    common::Error err = connection->Write(iAm, &nwrite);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
  } else if (IS_EQUAL_COMMAND(command, SERVER_PLEASE_SYSTEM_INFO_COMMAND)) {
    const common::system_info::CpuInfo& c1 = common::system_info::currentCpuInfo();
    std::string brand = c1.brandName();

    int64_t ram_total = common::system_info::amountOfPhysicalMemory();
    int64_t ram_free = common::system_info::amountOfAvailablePhysicalMemory();

    std::string os_name = common::system_info::operatingSystemName();
    std::string os_version = common::system_info::operatingSystemVersion();
    std::string os_arch = common::system_info::operatingSystemArchitecture();

    std::string os = common::MemSPrintf("%s %s(%s)", os_name, os_version, os_arch);

    json_object* info_json = json_object_new_object();
    json_object_object_add(info_json, STATUS_OS_FIELD, json_object_new_string(os.c_str()));
    json_object_object_add(info_json, STATUS_CPU_FIELD, json_object_new_string(brand.c_str()));
    json_object_object_add(info_json, STATUS_RAM_TOTAL_FIELD, json_object_new_int64(ram_total));
    json_object_object_add(info_json, STATUS_RAM_FREE_FIELD, json_object_new_int64(ram_free));

    const char* info_json_string = json_object_get_string(info_json);
    cmd_responce_t resp = SystemInfoResponceSuccsess(id, info_json_string);
    common::Error err = connection->Write(resp, &nwrite);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
    json_object_put(info_json);
  } else {
    WARNING_LOG() << "UNKNOWN REQUEST COMMAND: " << command;
  }
}

void InnerTcpHandler::HandleInnerResponceCommand(fasto::fastotv::inner::InnerClient* connection,
                                                 cmd_seq_t id,
                                                 int argc,
                                                 char* argv[]) {
  size_t nwrite = 0;
  char* state_command = argv[0];

  if (IS_EQUAL_COMMAND(state_command, SUCCESS_COMMAND) && argc > 1) {
    char* command = argv[1];
    if (IS_EQUAL_COMMAND(command, CLIENT_PING_COMMAND)) {
      if (argc > 2) {
        const char* pong = argv[2];
        if (!pong) {
          cmd_approve_t resp = PingApproveResponceFail(id, "Invalid input argument(s)");
          common::Error err = connection->Write(resp, &nwrite);
          if (err && err->IsError()) {
            DEBUG_MSG_ERROR(err);
          }
          return;
        }

        const cmd_approve_t resp = PingApproveResponceSuccsess(id);
        common::Error err = connection->Write(resp, &nwrite);
        if (err && err->IsError()) {
          DEBUG_MSG_ERROR(err);
          return;
        }
      } else {
        cmd_approve_t resp = PingApproveResponceFail(id, "Invalid input argument(s)");
        common::Error err = connection->Write(resp, &nwrite);
        if (err && err->IsError()) {
          DEBUG_MSG_ERROR(err);
        }
      }
    } else if (IS_EQUAL_COMMAND(command, CLIENT_GET_CHANNELS)) {
      if (argc > 2) {
        const char* hex_encoded_channels = argv[2];
        if (!hex_encoded_channels) {
          cmd_approve_t resp = GetChannelsApproveResponceFail(id, "Invalid input argument(s)");
          common::Error err = connection->Write(resp, &nwrite);
          if (err && err->IsError()) {
            DEBUG_MSG_ERROR(err);
          }
          return;
        }

        common::buffer_t buff = common::HexDecode(hex_encoded_channels);
        std::string buff_str(buff.begin(), buff.end());
        json_object* obj = json_tokener_parse(buff_str.c_str());
        if (!obj) {
          cmd_approve_t resp = GetChannelsApproveResponceFail(id, "Invalid input argument(s)");
          common::Error err = connection->Write(resp, &nwrite);
          if (err && err->IsError()) {
            DEBUG_MSG_ERROR(err);
          }
          return;
        }

        channels_t channels = MakeChannelsClass(obj);
        fApp->PostEvent(new core::events::ReceiveChannelsEvent(this, channels));
        const cmd_approve_t resp = GetChannelsApproveResponceSuccsess(id);
        common::Error err = connection->Write(resp, &nwrite);
        if (err && err->IsError()) {
          DEBUG_MSG_ERROR(err);
          return;
        }
      } else {
        cmd_approve_t resp = GetChannelsApproveResponceFail(id, "Invalid input argument(s)");
        common::Error err = connection->Write(resp, &nwrite);
        if (err && err->IsError()) {
          DEBUG_MSG_ERROR(err);
        }
      }
    } else {
      WARNING_LOG() << "UNKNOWN RESPONCE COMMAND: " << command;
    }
  } else if (IS_EQUAL_COMMAND(state_command, FAIL_COMMAND) && argc > 1) {
  } else {
    WARNING_LOG() << "UNKNOWN STATE COMMAND: " << state_command;
  }
}

void InnerTcpHandler::HandleInnerApproveCommand(fasto::fastotv::inner::InnerClient* connection,
                                                cmd_seq_t id,
                                                int argc,
                                                char* argv[]) {
  UNUSED(id);
  char* command = argv[0];

  if (IS_EQUAL_COMMAND(command, SUCCESS_COMMAND)) {
    if (argc > 1) {
      const char* okrespcommand = argv[1];
      if (IS_EQUAL_COMMAND(okrespcommand, SERVER_PING_COMMAND)) {
      } else if (IS_EQUAL_COMMAND(okrespcommand, SERVER_WHO_ARE_YOU_COMMAND)) {
        connection->SetName(ainf_.login);
        fApp->PostEvent(new core::events::ClientConnectedEvent(this, ainf_));
      }
    }
  } else if (IS_EQUAL_COMMAND(command, FAIL_COMMAND)) {
  } else {
    WARNING_LOG() << "UNKNOWN COMMAND: " << command;
  }
}

}  // namespace inner
}
}  // namespace fastotv
}  // namespace fasto
