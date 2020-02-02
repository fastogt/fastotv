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

#include "client/inner/inner_tcp_handler.h"

#include <algorithm>
#include <string>

#include <common/application/application.h>  // for fApp
#include <common/libev/io_loop.h>            // for IoLoop
#include <common/net/net.h>                  // for connect

#include "client/events/network_events.h"  // for BandwidtInfo, Con...

#include <fastotv/client/client.h>
#include <fastotv/commands/commands.h>
#include <fastotv/commands_info/channels_info.h>  // for ChannelsInfo
#include <fastotv/commands_info/server_info.h>    // for ServerInfo
#include <fastotv/commands_info/vods_info.h>

#define CHANNELS_ARRAY_FIELD "channels"
#define VODS_ARRAY_FIELD "vods"
#define PRIVATE_CHANNELS_ARRAY_FIELD "private_channels"

namespace fastotv {
namespace client {
namespace inner {

InnerTcpHandler::InnerTcpHandler(const common::net::HostAndPort& server_host, const commands_info::AuthInfo& auth_info)
    : common::libev::IoLoopObserver(),
      inner_connection_(nullptr),
      ping_server_id_timer_(INVALID_TIMER_ID),
      server_host_(server_host),
      auth_info_(auth_info) {}

InnerTcpHandler::~InnerTcpHandler() {
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
    Client* iclient = static_cast<Client*>(client);
    common::net::socket_info info = iclient->GetInfo();
    common::net::HostAndPort host(info.host(), info.port());
    events::ConnectInfo cinf(host);
    fApp->PostEvent(new events::ClientDisconnectedEvent(this, cinf));
    inner_connection_ = nullptr;
    return;
  }
}

void InnerTcpHandler::DataReceived(common::libev::IoClient* client) {
  if (client == inner_connection_) {
    std::string buff;
    Client* iclient = static_cast<Client*>(client);
    common::ErrnoError err = iclient->ReadCommand(&buff);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      ignore_result(client->Close());
      delete client;
      return;
    }

    HandleInnerDataReceived(iclient, buff);
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

  CHECK(!inner_connection_);
}

void InnerTcpHandler::TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) {
  UNUSED(server);
  if (id == ping_server_id_timer_ && inner_connection_) {
    Client* client = inner_connection_;
    common::ErrnoError err = client->Ping();
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      ignore_result(client->Close());
      delete client;
    }
  }
}

void InnerTcpHandler::Accepted(common::libev::IoChild* child) {
  UNUSED(child);
}

void InnerTcpHandler::Moved(common::libev::IoLoop* server, common::libev::IoChild* child) {
  UNUSED(server);
  UNUSED(child);
}

void InnerTcpHandler::ChildStatusChanged(common::libev::IoChild* child, int status, int signal) {
  UNUSED(child);
  UNUSED(status);
  UNUSED(signal);
}

void InnerTcpHandler::ActivateRequest() {
  if (!inner_connection_) {
    return;
  }

  Client* client = inner_connection_;
  common::ErrnoError err = client->Login(auth_info_);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    ignore_result(client->Close());
    delete client;
  }
}

void InnerTcpHandler::RequestServerInfo() {
  if (!inner_connection_) {
    return;
  }

  Client* client = inner_connection_;
  common::ErrnoError err = client->GetServerInfo();
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    ignore_result(client->Close());
    delete client;
  }
}

void InnerTcpHandler::RequestChannels() {
  if (!inner_connection_) {
    return;
  }

  Client* client = inner_connection_;
  common::ErrnoError err = client->GetChannels();
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    ignore_result(client->Close());
    delete client;
  }
}

void InnerTcpHandler::RequesRuntimeChannelInfo(stream_id_t sid) {
  if (!inner_connection_) {
    return;
  }

  Client* client = inner_connection_;
  common::ErrnoError err = client->GetRuntimeChannelInfo(sid);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    ignore_result(client->Close());
    delete client;
  }
}

void InnerTcpHandler::Connect(common::libev::IoLoop* server) {
  if (!server) {
    return;
  }

  DisConnect(common::make_error("Reconnect"));

  common::net::socket_info client_info;
  common::ErrnoError err = common::net::connect(server_host_, common::net::ST_SOCK_STREAM, nullptr, &client_info);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    events::ConnectInfo cinf(server_host_);
    auto ex_event =
        common::make_exception_event(new events::ClientConnectedEvent(this, cinf), common::make_error_from_errno(err));
    fApp->PostEvent(ex_event);
    return;
  }

  Client* connection = new Client(server, client_info);
  inner_connection_ = connection;
  server->RegisterClient(connection);
  events::ConnectInfo cinf(server_host_);
  fApp->PostEvent(new events::ClientConnectedEvent(this, cinf));
}

void InnerTcpHandler::DisConnect(common::Error err) {
  UNUSED(err);
  if (inner_connection_) {
    Client* connection = inner_connection_;
    ignore_result(connection->Close());
    delete connection;
  }
}

common::ErrnoError InnerTcpHandler::HandleRequestServerPing(Client* client, const protocol::request_t* req) {
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

    return client->Pong(req->id);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError InnerTcpHandler::HandleRequestServerClientInfo(Client* client, const protocol::request_t* req) {
  return client->SystemInfo(req->id, auth_info_.GetLogin(), auth_info_.GetDeviceID(),
                            commands_info::ProjectInfo(PROJECT_NAME_LOWERCASE, PROJECT_VERSION));
}

common::ErrnoError InnerTcpHandler::HandleRequestServerTextNotification(Client* client,
                                                                        const protocol::request_t* req) {
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jnotify_text = json_tokener_parse(params_ptr);
    if (!jnotify_text) {
      return common::make_errno_error_inval();
    }

    commands_info::NotificationTextInfo notification_text_info;
    common::Error err_des = notification_text_info.DeSerialize(jnotify_text);
    json_object_put(jnotify_text);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    fApp->PostEvent(new events::NotificationTextEvent(this, notification_text_info));
    return client->NotificationTextOK(req->id);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError InnerTcpHandler::HandleInnerDataReceived(Client* client, const std::string& input_command) {
  protocol::request_t* req = nullptr;
  protocol::response_t* resp = nullptr;
  common::Error err_parse = common::protocols::json_rpc::ParseJsonRPC(input_command, &req, &resp);
  if (err_parse) {
    const std::string err_str = err_parse->GetDescription();
    return common::make_errno_error(err_str, EAGAIN);
  }

  if (req) {
    DEBUG_LOG() << "Received request: " << input_command;
    common::ErrnoError err = HandleRequestCommand(client, req);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    delete req;
  } else if (resp) {
    DEBUG_LOG() << "Received responce: " << input_command;
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

common::ErrnoError InnerTcpHandler::HandleRequestCommand(Client* client, const protocol::request_t* req) {
  Client* sclient = static_cast<Client*>(client);
  if (req->method == SERVER_PING) {
    return HandleRequestServerPing(sclient, req);
  } else if (req->method == SERVER_GET_CLIENT_INFO) {
    return HandleRequestServerClientInfo(sclient, req);
  } else if (req->method == SERVER_TEXT_NOTIFICATION) {
    return HandleRequestServerTextNotification(sclient, req);
  }

  WARNING_LOG() << "Received unknown command: " << req->method;
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandler::HandleResponceClientActivateDevice(Client* client,
                                                                       const protocol::response_t* resp) {
  UNUSED(client);
  if (resp->IsMessage()) {
    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandler::HandleResponceClientLogin(Client* client, const protocol::response_t* resp) {
  if (resp->IsMessage()) {
    client->SetName(auth_info_.GetLogin());
    fApp->PostEvent(new events::ClientAuthorizedEvent(this, auth_info_));
    return common::ErrnoError();
  }

  common::Error err = common::make_error(resp->error->message);
  auto ex_event = common::make_exception_event(new events::ClientAuthorizedEvent(this, auth_info_), err);
  fApp->PostEvent(ex_event);
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandler::HandleResponceClientPing(Client* client, const protocol::response_t* resp) {
  UNUSED(client);
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jserver_ping = json_tokener_parse(params_ptr);
    if (!jserver_ping) {
      return common::make_errno_error_inval();
    }

    commands_info::ServerPingInfo server_ping_info;
    common::Error err_des = server_ping_info.DeSerialize(jserver_ping);
    json_object_put(jserver_ping);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }
    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandler::HandleResponceClientGetServerInfo(Client* client,
                                                                      const protocol::response_t* resp) {
  UNUSED(client);

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

    fApp->PostEvent(new events::ClientServerInfoEvent(this, sinf));
    return common::ErrnoError();
  }

  common::Error err = common::make_error(resp->error->message);
  auto ex_event =
      common::make_exception_event(new events::ClientServerInfoEvent(this, commands_info::ServerInfo()), err);
  fApp->PostEvent(ex_event);
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandler::HandleResponceClientGetChannels(Client* client, const protocol::response_t* resp) {
  UNUSED(client);
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jchannels_info = json_tokener_parse(params_ptr);
    if (!jchannels_info) {
      return common::make_errno_error_inval();
    }

    json_object* jchannels_array = nullptr;
    json_bool jchannels_array_exists =
        json_object_object_get_ex(jchannels_info, CHANNELS_ARRAY_FIELD, &jchannels_array);
    if (!jchannels_array_exists) {
      json_object_put(jchannels_info);
      return common::make_errno_error_inval();
    }

    json_object* jvods_array = nullptr;
    json_bool vods_array_exists = json_object_object_get_ex(jchannels_info, VODS_ARRAY_FIELD, &jvods_array);
    if (!vods_array_exists) {
      json_object_put(jchannels_info);
      return common::make_errno_error_inval();
    }

    json_object* jprivate_channels_array = nullptr;
    json_bool jprivate_channels_array_exists =
        json_object_object_get_ex(jchannels_info, PRIVATE_CHANNELS_ARRAY_FIELD, &jprivate_channels_array);
    if (!jprivate_channels_array_exists) {
      json_object_put(jchannels_info);
      return common::make_errno_error_inval();
    }

    commands_info::ChannelsInfo channels;
    common::Error err_des = channels.DeSerialize(jchannels_array);
    if (err_des) {
      json_object_put(jchannels_info);
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    commands_info::VodsInfo vods;
    err_des = vods.DeSerialize(jvods_array);
    if (err_des) {
      json_object_put(jchannels_info);
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    commands_info::ChannelsInfo private_channels;
    err_des = private_channels.DeSerialize(jprivate_channels_array);
    if (err_des) {
      json_object_put(jchannels_info);
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    json_object_put(jchannels_info);
    events::ChannelsMixInfo ch = {channels, vods, private_channels};
    fApp->PostEvent(new events::ReceiveChannelsEvent(this, ch));
    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandler::HandleResponceClientGetruntimeChannelInfo(Client* client,
                                                                              const protocol::response_t* resp) {
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

common::ErrnoError InnerTcpHandler::HandleResponceCommand(Client* client, const protocol::response_t* resp) {
  protocol::request_t req;
  Client* sclient = static_cast<Client*>(client);
  if (sclient->PopRequestByID(resp->id, &req)) {
    if (req.method == CLIENT_ACTIVATE_DEVICE) {
      return HandleResponceClientActivateDevice(sclient, resp);
    } else if (req.method == CLIENT_LOGIN) {
      return HandleResponceClientLogin(sclient, resp);
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
