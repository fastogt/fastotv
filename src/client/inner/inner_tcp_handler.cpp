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

#include "client/inner/inner_tcp_handler.h"

#include <algorithm>
#include <string>

#include <common/application/application.h>  // for fApp
#include <common/libev/io_loop.h>            // for IoLoop
#include <common/net/net.h>                  // for connect
#include <common/system_info/cpu_info.h>     // for CurrentCpuInfo
#include <common/system_info/system_info.h>  // for AmountOfAvailable...

#include "client/bandwidth/tcp_bandwidth_client.h"  // for TcpBandwidthClient
#include "client/commands.h"
#include "client/events/network_events.h"  // for BandwidtInfo, Con...

#include <fastotv/client.h>  // for Client

#include <fastotv/commands_info/channels_info.h>  // for ChannelsInfo
#include <fastotv/commands_info/client_info.h>    // for ClientInfo
#include <fastotv/commands_info/ping_info.h>      // for ClientPingInfo
#include <fastotv/commands_info/runtime_channel_info.h>
#include <fastotv/commands_info/server_info.h>  // for ServerInfo

namespace fastotv {
namespace client {
namespace inner {

class InnerTcpHandler::InnerSTBClient : public fastotv::ProtocoledClient {
 public:
  typedef fastotv::ProtocoledClient base_class;
  InnerSTBClient(common::libev::IoLoop* server, const common::net::socket_info& info) : base_class(server, info) {}
};

InnerTcpHandler::InnerTcpHandler(const StartConfig& config)
    : common::libev::IoLoopObserver(),
      inner_connection_(nullptr),
      bandwidth_requests_(),
      ping_server_id_timer_(INVALID_TIMER_ID),
      config_(config),
      current_bandwidth_(0),
      id_() {}

InnerTcpHandler::~InnerTcpHandler() {
  CHECK(bandwidth_requests_.empty());
  CHECK(!inner_connection_);
}

void InnerTcpHandler::PreLooped(common::libev::IoLoop* server) {
  ping_server_id_timer_ = server->CreateTimer(ping_timeout_server, true);

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
    InnerSTBClient* iclient = static_cast<InnerSTBClient*>(client);
    common::net::socket_info info = iclient->GetInfo();
    common::net::HostAndPort host(info.host(), info.port());
    events::ConnectInfo cinf(host);
    fApp->PostEvent(new events::ClientDisconnectedEvent(this, cinf));
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
  common::net::socket_info info = band_client->GetInfo();
  const common::net::HostAndPort host(info.host(), info.port());
  const BandwidthHostType hs = band_client->GetHostType();
  const bandwidth_t band = band_client->GetDownloadBytesPerSecond();
  if (hs == MAIN_SERVER) {
    current_bandwidth_ = band;
  }
  events::BandwidtInfo cinf(host, band, hs);
  events::BandwidthEstimationEvent* band_event = new events::BandwidthEstimationEvent(this, cinf);
  fApp->PostEvent(band_event);
}

void InnerTcpHandler::DataReceived(common::libev::IoClient* client) {
  if (client == inner_connection_) {
    std::string buff;
    InnerSTBClient* iclient = static_cast<InnerSTBClient*>(client);
    common::ErrnoError err = iclient->ReadCommand(&buff);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      err = client->Close();
      DCHECK(!err) << "Close client error: " << err->GetDescription();
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
  common::ErrnoError err = band_client->Read(buff, bandwidth::TcpBandwidthClient::max_payload_len, &nwread);
  if (err) {
    int e_code = err->GetErrorCode();
    if (e_code != EINTR) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    err = client->Close();
    DCHECK(!err) << "Close client error: " << err->GetDescription();
    delete client;
  }
}

void InnerTcpHandler::DataReadyToWrite(common::libev::IoClient* client) {
  UNUSED(client);
}

void InnerTcpHandler::PostLooped(common::libev::IoLoop* server) {
  UNUSED(server);
  if (ping_server_id_timer_ != INVALID_TIMER_ID) {
    server->RemoveTimer(ping_server_id_timer_);
    ping_server_id_timer_ = INVALID_TIMER_ID;
  }
  std::vector<bandwidth::TcpBandwidthClient*> copy = bandwidth_requests_;
  for (bandwidth::TcpBandwidthClient* ban : copy) {
    common::ErrnoError err = ban->Close();
    DCHECK(!err) << "Close client error: " << err->GetDescription();
    delete ban;
  }
  CHECK(bandwidth_requests_.empty());
  DisConnect(common::Error());
  CHECK(!inner_connection_);
}

void InnerTcpHandler::TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) {
  UNUSED(server);
  if (id == ping_server_id_timer_ && inner_connection_) {
    std::string ping_server_json;
    commands_info::ServerPingInfo server_ping_info;
    common::Error err_ser = server_ping_info.SerializeToString(&ping_server_json);
    if (err_ser) {
      return;
    }

    const protocol::request_t ping_request = PingRequest(NextRequestID(), ping_server_json);
    InnerSTBClient* client = inner_connection_;
    common::ErrnoError err = client->WriteRequest(ping_request);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      err = client->Close();
      DCHECK(!err) << "Close client error: " << err->GetDescription();
      delete client;
    }
  }
}

protocol::sequance_id_t InnerTcpHandler::NextRequestID() {
  const protocol::seq_id_t next_id = id_++;
  return common::protocols::json_rpc::MakeRequestID(next_id);
}

#if LIBEV_CHILD_ENABLE
void InnerTcpHandler::Accepted(common::libev::IoChild* child) {
  UNUSED(child);
}

void InnerTcpHandler::Moved(common::libev::IoLoop* server, common::libev::IoChild* child) {
  UNUSED(server);
  UNUSED(child);
}

void InnerTcpHandler::ChildStatusChanged(common::libev::IoChild* child, int status) {
  UNUSED(child);
  UNUSED(status);
}
#endif

void InnerTcpHandler::ActivateRequest() {
  if (!inner_connection_) {
    return;
  }

  std::string auth_str;
  common::Error err_ser = config_.ainf.SerializeToString(&auth_str);
  if (err_ser) {
    // const std::string err_str = err_ser->GetDescription();
    // return common::make_errno_error(err_str, EAGAIN);
    return;
  }

  InnerSTBClient* client = inner_connection_;
  const protocol::request_t channels_request = ActiveRequest(NextRequestID(), auth_str);
  common::ErrnoError err = client->WriteRequest(channels_request);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    err = client->Close();
    DCHECK(!err) << "Close client error: " << err->GetDescription();
    delete client;
  }
}

void InnerTcpHandler::RequestServerInfo() {
  if (!inner_connection_) {
    return;
  }

  const protocol::request_t channels_request = GetServerInfoRequest(NextRequestID());
  InnerSTBClient* client = inner_connection_;
  common::ErrnoError err = client->WriteRequest(channels_request);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    err = client->Close();
    DCHECK(!err) << "Close client error: " << err->GetDescription();
    delete client;
  }
}

void InnerTcpHandler::RequestChannels() {
  if (!inner_connection_) {
    return;
  }

  const protocol::request_t channels_request = GetChannelsRequest(NextRequestID());
  InnerSTBClient* client = inner_connection_;
  common::ErrnoError err = client->WriteRequest(channels_request);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    err = client->Close();
    DCHECK(!err) << "Close client error: " << err->GetDescription();
    delete client;
  }
}

void InnerTcpHandler::RequesRuntimeChannelInfo(stream_id sid) {
  if (!inner_connection_) {
    return;
  }

  std::string run_str;
  commands_info::RuntimeChannelLiteInfo run(sid);
  common::Error err_ser = run.SerializeToString(&run_str);
  if (err_ser) {
    DEBUG_MSG_ERROR(err_ser, common::logging::LOG_LEVEL_ERR);
    return;
  }

  const protocol::request_t channels_request = GetRuntimeChannelInfoRequest(NextRequestID(), run_str);
  InnerSTBClient* client = inner_connection_;
  common::ErrnoError err = client->WriteRequest(channels_request);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    err = client->Close();
    DCHECK(!err) << "Close client error: " << err->GetDescription();
    delete client;
  }
}

void InnerTcpHandler::Connect(common::libev::IoLoop* server) {
  if (!server) {
    return;
  }

  DisConnect(common::make_error("Reconnect"));

  common::net::HostAndPort host = config_.inner_host;
  common::net::socket_info client_info;
  common::ErrnoError err = common::net::connect(host, common::net::ST_SOCK_STREAM, nullptr, &client_info);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    events::ConnectInfo cinf(host);
    auto ex_event =
        common::make_exception_event(new events::ClientConnectedEvent(this, cinf), common::make_error_from_errno(err));
    fApp->PostEvent(ex_event);
    return;
  }

  InnerSTBClient* connection = new InnerSTBClient(server, client_info);
  inner_connection_ = connection;
  server->RegisterClient(connection);
  events::ConnectInfo cinf(host);
  fApp->PostEvent(new events::ClientConnectedEvent(this, cinf));
}

void InnerTcpHandler::DisConnect(common::Error err) {
  UNUSED(err);
  if (inner_connection_) {
    InnerSTBClient* connection = inner_connection_;
    common::ErrnoError errn = connection->Close();
    DCHECK(!errn) << "Close connection error: " << errn->GetDescription();
    delete connection;
  }
}

common::ErrnoError InnerTcpHandler::CreateAndConnectTcpBandwidthClient(common::libev::IoLoop* server,
                                                                       const common::net::HostAndPort& host,
                                                                       BandwidthHostType hs,
                                                                       bandwidth::TcpBandwidthClient** out_band) {
  if (!server || !out_band) {
    return common::make_errno_error_inval();
  }

  common::net::socket_info client_info;
  common::ErrnoError err = common::net::connect(host, common::net::ST_SOCK_STREAM, nullptr, &client_info);
  if (err) {
    return err;
  }

  bandwidth::TcpBandwidthClient* connection = new bandwidth::TcpBandwidthClient(server, client_info, hs);
  err = connection->StartSession(0, 1000);
  if (err) {
    common::ErrnoError err_close = connection->Close();
    DCHECK(!err_close) << "Close connection error: " << err_close->GetDescription();
    delete connection;
    return err;
  }

  *out_band = connection;
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandler::HandleRequestServerPing(InnerSTBClient* client, protocol::request_t* req) {
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jstop = json_tokener_parse(params_ptr);
    if (!jstop) {
      return common::make_errno_error_inval();
    }

    commands_info::ServerPingInfo server_ping_info;
    common::Error err_des = server_ping_info.DeSerialize(jstop);
    json_object_put(jstop);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    protocol::response_t resp = PingResponseSuccess(req->id);
    return client->WriteResponce(resp);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError InnerTcpHandler::HandleRequestServerClientInfo(InnerSTBClient* client, protocol::request_t* req) {
  const common::system_info::CpuInfo& c1 = common::system_info::CurrentCpuInfo();
  std::string brand = c1.GetBrandName();

  int64_t ram_total = common::system_info::AmountOfPhysicalMemory();
  int64_t ram_free = common::system_info::AmountOfAvailablePhysicalMemory();

  std::string os_name = common::system_info::OperatingSystemName();
  std::string os_version = common::system_info::OperatingSystemVersion();
  std::string os_arch = common::system_info::OperatingSystemArchitecture();

  std::string os = common::MemSPrintf("%s %s(%s)", os_name, os_version, os_arch);

  commands_info::ClientInfo info(config_.ainf.GetLogin(), os, brand, ram_total, ram_free, current_bandwidth_);
  std::string info_json_string;
  common::Error err_ser = info.SerializeToString(&info_json_string);
  if (err_ser) {
    const std::string err_str = err_ser->GetDescription();
    return common::make_errno_error(err_str, EAGAIN);
  }

  const protocol::response_t resp = SystemInfoResponceSuccsess(req->id, info_json_string);
  return client->WriteResponce(resp);
}

common::ErrnoError InnerTcpHandler::HandleInnerDataReceived(InnerSTBClient* client, const std::string& input_command) {
  protocol::request_t* req = nullptr;
  protocol::response_t* resp = nullptr;
  common::Error err_parse = common::protocols::json_rpc::ParseJsonRPC(input_command, &req, &resp);
  if (err_parse) {
    const std::string err_str = err_parse->GetDescription();
    return common::make_errno_error(err_str, EAGAIN);
  }

  if (req) {
    INFO_LOG() << "Received request: " << input_command;
    common::ErrnoError err = HandleRequestCommand(client, req);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    delete req;
  } else if (resp) {
    INFO_LOG() << "Received responce: " << input_command;
    common::ErrnoError err = HandleResponceCommand(client, resp);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    delete resp;
  } else {
    DNOTREACHED();
    return common::make_errno_error("Invalid command type.", EINVAL);
  }

  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandler::HandleRequestCommand(InnerSTBClient* client, protocol::request_t* req) {
  InnerSTBClient* sclient = static_cast<InnerSTBClient*>(client);
  if (req->method == SERVER_PING) {
    return HandleRequestServerPing(sclient, req);
  } else if (req->method == SERVER_GET_CLIENT_INFO) {
    return HandleRequestServerClientInfo(sclient, req);
  }

  WARNING_LOG() << "Received unknown command: " << req->method;
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandler::HandleResponceClientActivate(InnerSTBClient* client, protocol::response_t* resp) {
  if (resp->IsMessage()) {
    client->SetName(config_.ainf.GetLogin());
    fApp->PostEvent(new events::ClientAuthorizedEvent(this, config_.ainf));
    return common::ErrnoError();
  }

  common::Error err = common::make_error(resp->error->message);
  auto ex_event = common::make_exception_event(new events::ClientAuthorizedEvent(this, config_.ainf), err);
  fApp->PostEvent(ex_event);
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandler::HandleResponceClientPing(InnerSTBClient* client, protocol::response_t* resp) {
  UNUSED(client);
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jclient_ping = json_tokener_parse(params_ptr);
    if (!jclient_ping) {
      return common::make_errno_error_inval();
    }

    commands_info::ClientPingInfo client_ping_info;
    common::Error err_des = client_ping_info.DeSerialize(jclient_ping);
    json_object_put(jclient_ping);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }
    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandler::HandleResponceClientGetServerInfo(InnerSTBClient* client,
                                                                      protocol::response_t* resp) {
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jserver_info = json_tokener_parse(params_ptr);
    if (!jserver_info) {
      return common::make_errno_error_inval();
    }

    commands_info::ServerInfo sinf;
    common::Error err_des = sinf.DeSerialize(jserver_info);
    json_object_put(jserver_info);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    common::net::HostAndPort host = sinf.GetBandwidthHost();
    bandwidth::TcpBandwidthClient* band_connection = nullptr;
    common::libev::IoLoop* server = client->GetServer();
    const BandwidthHostType hs = MAIN_SERVER;
    common::ErrnoError errn = CreateAndConnectTcpBandwidthClient(server, host, hs, &band_connection);
    if (errn) {
      events::BandwidtInfo cinf(host, 0, hs);
      current_bandwidth_ = 0;
      auto ex_event = common::make_exception_event(new events::BandwidthEstimationEvent(this, cinf),
                                                   common::make_error_from_errno(errn));
      fApp->PostEvent(ex_event);
      return errn;
    }

    bandwidth_requests_.push_back(band_connection);
    server->RegisterClient(band_connection);
    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandler::HandleResponceClientGetChannels(InnerSTBClient* client,
                                                                    protocol::response_t* resp) {
  UNUSED(client);
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jchannels_info = json_tokener_parse(params_ptr);
    if (!jchannels_info) {
      return common::make_errno_error_inval();
    }

    commands_info::ChannelsInfo chan;
    common::Error err_des = chan.DeSerialize(jchannels_info);
    json_object_put(jchannels_info);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    fApp->PostEvent(new events::ReceiveChannelsEvent(this, chan));
    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandler::HandleResponceClientGetruntimeChannelInfo(InnerSTBClient* client,
                                                                              protocol::response_t* resp) {
  UNUSED(client);
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jchannels_info = json_tokener_parse(params_ptr);
    if (!jchannels_info) {
      return common::make_errno_error_inval();
    }

    commands_info::RuntimeChannelInfo chan;
    common::Error err_des = chan.DeSerialize(jchannels_info);
    json_object_put(jchannels_info);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    fApp->PostEvent(new events::ReceiveRuntimeChannelEvent(this, chan));
    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandler::HandleResponceCommand(InnerSTBClient* client, protocol::response_t* resp) {
  protocol::request_t req;
  InnerSTBClient* sclient = static_cast<InnerSTBClient*>(client);
  if (sclient->PopRequestByID(resp->id, &req)) {
    if (req.method == CLIENT_ACTIVATE) {
      return HandleResponceClientActivate(sclient, resp);
    } else if (req.method == CLIENT_PING) {
      return HandleResponceClientPing(sclient, resp);
    } else if (req.method == CLIENT_GET_SERVER_INFO) {
      return HandleResponceClientGetServerInfo(sclient, resp);
    } else if (req.method == CLIENT_GET_CHANNELS) {
      return HandleResponceClientGetChannels(sclient, resp);
    } else if (req.method == CLIENT_GET_RUNTIME_CHANNEL_INFO) {
      return HandleResponceClientGetruntimeChannelInfo(sclient, resp);
    } else {
      WARNING_LOG() << "HandleResponceServiceCommand not handled command: " << req.method;
    }
  }

  return common::ErrnoError();
}

}  // namespace inner
}  // namespace client
}  // namespace fastotv
