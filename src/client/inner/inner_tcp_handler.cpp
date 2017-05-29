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

#include <common/libev/tcp/tcp_server.h>

#include "inner/inner_client.h"

#include "server_info.h"
#include "client_info.h"
#include "ping_info.h"

#include "client/commands.h"
#include "client/core/events/network_events.h"
#include "client/bandwidth/tcp_bandwidth_client.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace inner {

InnerTcpHandler::InnerTcpHandler(const StartConfig& config)
    : inner_connection_(nullptr),
      bandwidth_requests_(),
      ping_server_id_timer_(INVALID_TIMER_ID),
      config_(config),
      current_bandwidth_(0) {
}

InnerTcpHandler::~InnerTcpHandler() {
  for (bandwidth::TcpBandwidthClient* ban : bandwidth_requests_) {
    delete ban;
  }
  bandwidth_requests_.clear();
  delete inner_connection_;
  inner_connection_ = nullptr;
}

void InnerTcpHandler::PreLooped(common::libev::IoLoop* server) {
  ping_server_id_timer_ = server->CreateTimer(ping_timeout_server, ping_timeout_server);
  CHECK(!inner_connection_);

  Connect(server);
}

void InnerTcpHandler::Accepted(common::libev::IoClient* client) {
  UNUSED(client);
}

void InnerTcpHandler::Moved(common::libev::IoLoop* server, common::libev::IoClient* client) {
  UNUSED(server);
  UNUSED(client);
}

void InnerTcpHandler::Closed(common::libev::IoClient* client) {
  if (client == inner_connection_) {
    fasto::fastotv::inner::InnerClient* iclient =
        static_cast<fasto::fastotv::inner::InnerClient*>(client);
    common::net::socket_info info = iclient->Info();
    common::net::HostAndPort host(info.host(), info.port());
    core::events::ConnectInfo cinf(host);
    fApp->PostEvent(new core::events::ClientDisconnectedEvent(this, cinf));
    inner_connection_ = nullptr;
    return;
  }

  // bandwidth
  bandwidth::TcpBandwidthClient* band_client = static_cast<bandwidth::TcpBandwidthClient*>(client);
  auto it = std::remove(bandwidth_requests_.begin(), bandwidth_requests_.end(), band_client);
  if (it == bandwidth_requests_.end()) {
    return;
  }

  bandwidth_requests_.erase(it);
  common::net::socket_info info = band_client->Info();
  const common::net::HostAndPort host(info.host(), info.port());
  const BandwidthHostType hs = band_client->HostType();
  const bandwidth_t band = band_client->DownloadBytesPerSecond();
  if (hs == MAIN_SERVER) {
    current_bandwidth_ = band;
  }
  core::events::BandwidtInfo cinf(host, band, hs);
  core::events::BandwidthEstimationEvent* band_event =
      new core::events::BandwidthEstimationEvent(this, cinf);
  fApp->PostEvent(band_event);
}

void InnerTcpHandler::DataReceived(common::libev::IoClient* client) {
  if (client == inner_connection_) {
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
    return;
  }

  // bandwidth
  bandwidth::TcpBandwidthClient* band_client = static_cast<bandwidth::TcpBandwidthClient*>(client);
  char buff[bandwidth::TcpBandwidthClient::max_payload_len];
  size_t nwread;
  common::Error err =
      band_client->Read(buff, bandwidth::TcpBandwidthClient::max_payload_len, &nwread);
  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
    client->Close();
    delete client;
    return;
  }
}

void InnerTcpHandler::DataReadyToWrite(common::libev::IoClient* client) {
  UNUSED(client);
}

void InnerTcpHandler::PostLooped(common::libev::IoLoop* server) {
  UNUSED(server);
  for (bandwidth::TcpBandwidthClient* ban : bandwidth_requests_) {
    ban->Close();
  }
  DisConnect(common::Error());
}

void InnerTcpHandler::TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) {
  UNUSED(server);
  if (id == ping_server_id_timer_ && inner_connection_) {
    const cmd_request_t ping_request = PingRequest(NextRequestID());
    fasto::fastotv::inner::InnerClient* client = inner_connection_;
    common::Error err = client->Write(ping_request);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
      client->Close();
      delete client;
    }
  }
}

void InnerTcpHandler::RequestServerInfo() {
  if (!inner_connection_) {
    return;
  }

  const cmd_request_t channels_request = GetServerInfoRequest(NextRequestID());
  fasto::fastotv::inner::InnerClient* client = inner_connection_;
  common::Error err = client->Write(channels_request);
  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
    client->Close();
    delete client;
  }
}

void InnerTcpHandler::RequestChannels() {
  if (!inner_connection_) {
    return;
  }

  const cmd_request_t channels_request = GetChannelsRequest(NextRequestID());
  fasto::fastotv::inner::InnerClient* client = inner_connection_;
  common::Error err = client->Write(channels_request);
  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
    client->Close();
    delete client;
  }
}

void InnerTcpHandler::Connect(common::libev::IoLoop* server) {
  if (!server) {
    return;
  }

  DisConnect(common::make_error_value("Reconnect", common::Value::E_ERROR));

  common::net::HostAndPort host = config_.inner_host;
  common::net::socket_info client_info;
  common::ErrnoError err = common::net::connect(host, common::net::ST_SOCK_STREAM, 0, &client_info);
  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
    core::events::ConnectInfo cinf(host);
    auto ex_event = make_exception_event(new core::events::ClientConnectedEvent(this, cinf), err);
    fApp->PostEvent(ex_event);
    return;
  }

  fasto::fastotv::inner::InnerClient* connection =
      new fasto::fastotv::inner::InnerClient(server, client_info);
  inner_connection_ = connection;
  server->RegisterClient(connection);
}

void InnerTcpHandler::DisConnect(common::Error err) {
  UNUSED(err);
  if (inner_connection_) {
    fasto::fastotv::inner::InnerClient* connection = inner_connection_;
    connection->Close();
    delete connection;
  }
}

common::Error InnerTcpHandler::CreateAndConnectTcpBandwidthClient(
    common::libev::IoLoop* server,
    const common::net::HostAndPort& host,
    BandwidthHostType hs,
    bandwidth::TcpBandwidthClient** out_band) {
  if (!server || !out_band) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  common::net::socket_info client_info;
  common::Error err = common::net::connect(host, common::net::ST_SOCK_STREAM, 0, &client_info);
  if (err && err->IsError()) {
    return err;
  }

  bandwidth::TcpBandwidthClient* connection =
      new bandwidth::TcpBandwidthClient(server, client_info, hs);
  err = connection->StartSession(0, 1000);
  if (err && err->IsError()) {
    connection->Close();
    delete connection;
    return err;
  }

  *out_band = connection;
  return common::Error();
}

void InnerTcpHandler::HandleInnerRequestCommand(fasto::fastotv::inner::InnerClient* connection,
                                                cmd_seq_t id,
                                                int argc,
                                                char* argv[]) {
  UNUSED(argc);
  char* command = argv[0];

  if (IS_EQUAL_COMMAND(command, SERVER_PING_COMMAND)) {
    ServerPingInfo ping;
    json_object* jping = ServerPingInfo::MakeJobject(ping);
    std::string ping_str = json_object_get_string(jping);
    std::string enc_ping = Encode(ping_str);
    json_object_put(jping);
    const cmd_responce_t pong = PingResponceSuccsess(id, enc_ping);
    common::Error err = connection->Write(pong);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
    return;
  } else if (IS_EQUAL_COMMAND(command, SERVER_WHO_ARE_YOU_COMMAND)) {
    json_object* jauth = AuthInfo::MakeJobject(config_.ainf);
    std::string auth_str = json_object_get_string(jauth);
    std::string enc_auth = Encode(auth_str);
    json_object_put(jauth);
    cmd_responce_t iAm = WhoAreYouResponceSuccsess(id, enc_auth);
    common::Error err = connection->Write(iAm);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
    return;
  } else if (IS_EQUAL_COMMAND(command, SERVER_GET_CLIENT_INFO_COMMAND)) {
    const common::system_info::CpuInfo& c1 = common::system_info::CurrentCpuInfo();
    std::string brand = c1.BrandName();

    int64_t ram_total = common::system_info::AmountOfPhysicalMemory();
    int64_t ram_free = common::system_info::AmountOfAvailablePhysicalMemory();

    std::string os_name = common::system_info::OperatingSystemName();
    std::string os_version = common::system_info::OperatingSystemVersion();
    std::string os_arch = common::system_info::OperatingSystemArchitecture();

    std::string os = common::MemSPrintf("%s %s(%s)", os_name, os_version, os_arch);

    ClientInfo info;
    info.login = config_.ainf.login;
    info.os = os;
    info.cpu_brand = brand;
    info.ram_total = ram_total;
    info.ram_free = ram_free;
    info.bandwidth = current_bandwidth_;
    json_object* info_json = ClientInfo::MakeJobject(info);
    std::string info_json_string = json_object_get_string(info_json);
    std::string enc_info = Encode(info_json_string);
    cmd_responce_t resp = SystemInfoResponceSuccsess(id, enc_info);
    common::Error err = connection->Write(resp);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
    json_object_put(info_json);
    return;
  }

  WARNING_LOG() << "UNKNOWN REQUEST COMMAND: " << command;
}

void InnerTcpHandler::HandleInnerResponceCommand(fasto::fastotv::inner::InnerClient* connection,
                                                 cmd_seq_t id,
                                                 int argc,
                                                 char* argv[]) {
  char* state_command = argv[0];

  if (IS_EQUAL_COMMAND(state_command, SUCCESS_COMMAND) && argc > 1) {
    common::Error err = HandleInnerSuccsessResponceCommand(connection, id, argc, argv);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
    return;
  } else if (IS_EQUAL_COMMAND(state_command, FAIL_COMMAND) && argc > 1) {
    common::Error err = HandleInnerFailedResponceCommand(connection, id, argc, argv);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
    return;
  }

  WARNING_LOG() << "UNKNOWN STATE COMMAND: " << state_command;
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
        connection->SetName(config_.ainf.login);
        fApp->PostEvent(new core::events::ClientAuthorizedEvent(this, config_.ainf));
      } else if (IS_EQUAL_COMMAND(okrespcommand, SERVER_GET_CLIENT_INFO_COMMAND)) {
      }
    }
    return;
  } else if (IS_EQUAL_COMMAND(command, FAIL_COMMAND)) {
    if (argc > 1) {
      const char* failed_resp_command = argv[1];
      if (IS_EQUAL_COMMAND(failed_resp_command, SERVER_PING_COMMAND)) {
      } else if (IS_EQUAL_COMMAND(failed_resp_command, SERVER_WHO_ARE_YOU_COMMAND)) {
        common::Error err =
            common::make_error_value(argc > 2 ? argv[2] : "Unknown", common::Value::E_ERROR);
        auto ex_event =
            make_exception_event(new core::events::ClientAuthorizedEvent(this, config_.ainf), err);
        fApp->PostEvent(ex_event);
      } else if (IS_EQUAL_COMMAND(failed_resp_command, SERVER_GET_CLIENT_INFO_COMMAND)) {
      }
    }
    return;
  }

  WARNING_LOG() << "UNKNOWN COMMAND: " << command;
}

common::Error InnerTcpHandler::HandleInnerSuccsessResponceCommand(
    fastotv::inner::InnerClient* connection,
    cmd_seq_t id,
    int argc,
    char* argv[]) {
  char* command = argv[1];
  if (IS_EQUAL_COMMAND(command, CLIENT_PING_COMMAND)) {
    json_object* obj = NULL;
    common::Error parse_err = ParserResponceResponceCommand(argc, argv, &obj);
    if (parse_err && parse_err->IsError()) {
      cmd_approve_t resp = PingApproveResponceFail(id, parse_err->Description());
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return parse_err;
    }

    ClientPingInfo ping_info = ClientPingInfo::MakeClass(obj);
    UNUSED(ping_info);
    json_object_put(obj);
    cmd_approve_t resp = PingApproveResponceSuccsess(id);
    return connection->Write(resp);
  } else if (IS_EQUAL_COMMAND(command, CLIENT_GET_SERVER_INFO)) {
    json_object* obj = NULL;
    common::Error parse_err = ParserResponceResponceCommand(argc, argv, &obj);
    if (parse_err && parse_err->IsError()) {
      cmd_approve_t resp = GetServerInfoApproveResponceFail(id, parse_err->Description());
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return parse_err;
    }

    ServerInfo sinf = ServerInfo::MakeClass(obj);
    json_object_put(obj);

    common::net::HostAndPort host = sinf.bandwidth_host;
    bandwidth::TcpBandwidthClient* band_connection = NULL;
    common::libev::IoLoop* server = connection->Server();
    const BandwidthHostType hs = MAIN_SERVER;
    common::Error err = CreateAndConnectTcpBandwidthClient(server, host, hs, &band_connection);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
      core::events::BandwidtInfo cinf(host, 0, hs);
      current_bandwidth_ = 0;
      auto ex_event =
          make_exception_event(new core::events::BandwidthEstimationEvent(this, cinf), err);
      fApp->PostEvent(ex_event);
      return err;
    }

    bandwidth_requests_.push_back(band_connection);
    server->RegisterClient(band_connection);
    return common::Error();
  } else if (IS_EQUAL_COMMAND(command, CLIENT_GET_CHANNELS)) {
    json_object* obj = NULL;
    common::Error parse_err = ParserResponceResponceCommand(argc, argv, &obj);
    if (parse_err && parse_err->IsError()) {
      cmd_approve_t resp = GetChannelsApproveResponceFail(id, parse_err->Description());
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return parse_err;
    }

    channels_t channels = MakeChannelsClass(obj);
    json_object_put(obj);
    fApp->PostEvent(new core::events::ReceiveChannelsEvent(this, channels));
    const cmd_approve_t resp = GetChannelsApproveResponceSuccsess(id);
    return connection->Write(resp);
  }

  const std::string error_str = common::MemSPrintf("UNKNOWN RESPONCE COMMAND: %s", command);
  return common::make_error_value(error_str, common::Value::E_ERROR);
}

common::Error InnerTcpHandler::HandleInnerFailedResponceCommand(
    fastotv::inner::InnerClient* connection,
    cmd_seq_t id,
    int argc,
    char* argv[]) {
  UNUSED(connection);
  UNUSED(id);
  UNUSED(argc);

  char* command = argv[1];
  const std::string error_str =
      common::MemSPrintf("Sorry now we can't handle failed pesponce for command: %s", command);
  return common::make_error_value(error_str, common::Value::E_ERROR);
}

common::Error InnerTcpHandler::ParserResponceResponceCommand(int argc,
                                                             char* argv[],
                                                             json_object** out) {
  if (argc < 2) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  const char* arg_2_str = argv[2];
  if (!arg_2_str) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  std::string raw = Decode(arg_2_str);
  json_object* obj = json_tokener_parse(raw.c_str());
  if (!obj) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  *out = obj;
  return common::Error();
}

}  // namespace inner
}
}  // namespace fastotv
}  // namespace fasto
