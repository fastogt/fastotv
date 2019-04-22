/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

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

#include "server/inner/inner_tcp_handler.h"

#include <string>  // for string
#include <vector>

#include <common/libev/io_client.h>         // for IoClient
#include <common/libev/io_loop.h>           // for IoLoop
#include <common/logger.h>                  // for COMPACT_LOG_WARNING
#include <common/threads/thread_manager.h>  // for THREAD_MANAGER

#include "client_server_types.h"          // for Encode
#include "commands_info/auth_info.h"      // for AuthInfo
#include "commands_info/channels_info.h"  // for ChannelsInfo
#include "commands_info/client_info.h"    // for ClientInfo
#include "commands_info/ping_info.h"      // for ClientPingInfo
#include "inner/inner_client.h"           // for InnerClient

#include "server/commands.h"

#include "server/redis/redis_pub_sub.h"

#include "server/inner/inner_external_notifier.h"  // for InnerSubHandler
#include "server/inner/inner_tcp_client.h"         // for InnerTcpClient

#include "commands_info/runtime_channel_info.h"
#include "commands_info/server_info.h"  // for ServerInfo
#include "server/server_host.h"         // for ServerHost
#include "server/user_info.h"

// ui notifications
#define CLIENT_STATE "client_state"
#define CLIENT_CONNECTED_STATE "connected"

namespace fastotv {
namespace server {
namespace inner {

namespace {
protocol::request_t MakeClientStateNotification(bool connected) {
  json_object* obj = json_object_new_object();
  json_object_object_add(obj, CLIENT_CONNECTED_STATE, json_object_new_boolean(connected));
  const std::string state_str = json_object_get_string(obj);
  json_object_put(obj);
  return protocol::request_t::MakeNotification(CLIENT_STATE, state_str);
}
}  // namespace

InnerTcpHandlerHost::InnerTcpHandlerHost(ServerHost* parent, const Config& config)
    : parent_(parent),
      sub_commands_in_(nullptr),
      handler_(nullptr),
      ping_client_id_timer_(INVALID_TIMER_ID),
      reread_cache_id_timer_(INVALID_TIMER_ID),
      config_(config),
      chat_channels_() {
  handler_ = new InnerSubHandler(this);
  sub_commands_in_ = new redis::RedisPubSub(handler_);
  redis_subscribe_command_in_thread_ = THREAD_MANAGER()->CreateThread(&redis::RedisPubSub::Listen, sub_commands_in_);

  sub_commands_in_->SetConfig(config.server.redis);
  bool result = redis_subscribe_command_in_thread_->Start();
  if (!result) {
    WARNING_LOG() << "Don't started listen thread for external commands.";
  }
}

InnerTcpHandlerHost::~InnerTcpHandlerHost() {
  sub_commands_in_->Stop();
  redis_subscribe_command_in_thread_->Join();
  delete sub_commands_in_;
  delete handler_;
}

void InnerTcpHandlerHost::PreLooped(common::libev::IoLoop* server) {
  UpdateCache();
  ping_client_id_timer_ = server->CreateTimer(ping_timeout_clients, true);
  reread_cache_id_timer_ = server->CreateTimer(reread_cache_timeout, true);
}

void InnerTcpHandlerHost::Moved(common::libev::IoLoop* server, common::libev::IoClient* client) {
  UNUSED(server);
  UNUSED(client);
}

void InnerTcpHandlerHost::PostLooped(common::libev::IoLoop* server) {
  if (ping_client_id_timer_ != INVALID_TIMER_ID) {
    server->RemoveTimer(ping_client_id_timer_);
    ping_client_id_timer_ = INVALID_TIMER_ID;
  }

  if (reread_cache_id_timer_ != INVALID_TIMER_ID) {
    server->RemoveTimer(reread_cache_id_timer_);
    reread_cache_id_timer_ = INVALID_TIMER_ID;
  }
}

void InnerTcpHandlerHost::TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) {
  if (ping_client_id_timer_ == id) {
    std::vector<common::libev::IoClient*> online_clients = server->GetClients();
    for (size_t i = 0; i < online_clients.size(); ++i) {
      common::libev::IoClient* client = online_clients[i];
      InnerTcpClient* iclient = static_cast<InnerTcpClient*>(client);
      if (iclient) {
        std::string ping_server_json;
        ServerPingInfo server_ping_info;
        common::Error err_ser = server_ping_info.SerializeToString(&ping_server_json);
        if (err_ser) {
          continue;
        }

        const protocol::request_t ping_request = PingRequest(NextRequestID(), ping_server_json);
        common::ErrnoError err = iclient->WriteRequest(ping_request);
        if (err) {
          DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
          err = client->Close();
          DCHECK(!err) << "Close client error: " << err->GetDescription();
          delete client;
        } else {
          INFO_LOG() << "Sent ping to client[" << client->GetFormatedName() << "], from server["
                     << server->GetFormatedName() << "], " << online_clients.size() << " client(s) connected.";
        }
      }
    }
  } else if (reread_cache_id_timer_ == id) {
    UpdateCache();
  }
}

#if LIBEV_CHILD_ENABLE
void InnerTcpHandlerHost::Accepted(common::libev::IoChild* child) {
  UNUSED(child);
}

void InnerTcpHandlerHost::Moved(common::libev::IoLoop* server, common::libev::IoChild* child) {
  UNUSED(server);
  UNUSED(child);
}

void InnerTcpHandlerHost::ChildStatusChanged(common::libev::IoChild* client, int status) {
  UNUSED(client);
  UNUSED(status);
}
#endif

void InnerTcpHandlerHost::Accepted(common::libev::IoClient* client) {
  UNUSED(client);
}

void InnerTcpHandlerHost::Closed(common::libev::IoClient* client) {
  InnerTcpClient* iclient = static_cast<InnerTcpClient*>(client);
  common::libev::IoLoop* server = client->GetServer();
  const ServerAuthInfo server_user_auth = iclient->GetServerHostInfo();
  SendLeaveChatMessage(server, iclient->GetCurrentStreamID(), server_user_auth.GetLogin());

  if (iclient->IsAnonimUser()) {  // anonim user
    INFO_LOG() << "Byu anonim user: " << server_user_auth.GetLogin();
    return;
  }

  common::Error unreg_err = parent_->UnRegisterInnerConnectionByHost(iclient);
  if (unreg_err) {
    return;
  }

  const rpc::UserRpcInfo user_rpc = server_user_auth.MakeUserRpc();
  PublishUserStateInfo(user_rpc, false);
  INFO_LOG() << "Byu registered user: " << server_user_auth.GetLogin();
}

void InnerTcpHandlerHost::DataReceived(common::libev::IoClient* client) {
  std::string buff;
  InnerTcpClient* iclient = static_cast<InnerTcpClient*>(client);
  common::ErrnoError err = iclient->ReadCommand(&buff);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    err = client->Close();
    DCHECK(!err) << "Close client error: " << err->GetDescription();
    delete client;
    return;
  }

  HandleInnerDataReceived(iclient, buff);
}

void InnerTcpHandlerHost::DataReadyToWrite(common::libev::IoClient* client) {
  UNUSED(client);
}

common::Error InnerTcpHandlerHost::PublishToChannelOut(const std::string& msg) {
  return sub_commands_in_->PublishToChannelOut(msg);
}

void InnerTcpHandlerHost::UpdateCache() {
  std::vector<stream_id> channels;
  common::Error err = parent_->GetChatChannels(&channels);
  if (err) {
    return;
  }
  chat_channels_ = channels;
}

void InnerTcpHandlerHost::PublishUserStateInfo(const rpc::UserRpcInfo& user, bool connected) {
  std::string user_state_str;
  rpc::UserRequestInfo req(user.GetUserID(), user.GetDeviceID(), MakeClientStateNotification(connected));
  common::Error err = req.SerializeToString(&user_state_str);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    return;
  }

  err = sub_commands_in_->PublishStateToChannel(user_state_str);
  if (err) {
    WARNING_LOG() << "Publish message: " << user_state_str << " to channel clients state failed.";
  }
}

inner::InnerTcpClient* InnerTcpHandlerHost::FindInnerConnectionByUser(const rpc::UserRpcInfo& user) const {
  return parent_->FindInnerConnectionByUser(user);
}

void InnerTcpHandlerHost::SendEnterChatMessage(common::libev::IoLoop* server, stream_id sid, login_t login) {
  BrodcastChatMessage(server, MakeEnterMessage(sid, login));
}

void InnerTcpHandlerHost::SendLeaveChatMessage(common::libev::IoLoop* server, stream_id sid, login_t login) {
  BrodcastChatMessage(server, MakeLeaveMessage(sid, login));
}

void InnerTcpHandlerHost::BrodcastChatMessage(common::libev::IoLoop* server, const ChatMessage& msg) {
  std::string msg_ser;
  common::Error err = msg.SerializeToString(&msg_ser);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    return;
  }

  std::vector<common::libev::IoClient*> online_clients = server->GetClients();
  for (size_t i = 0; i < online_clients.size(); ++i) {
    common::libev::IoClient* client = online_clients[i];
    InnerTcpClient* iclient = static_cast<InnerTcpClient*>(client);
    if (iclient && iclient->GetCurrentStreamID() == msg.GetChannelID()) {
      const protocol::request_t message_request = ServerSendChatMessageRequest(NextRequestID(), msg_ser);
      common::ErrnoError errn = iclient->WriteRequest(message_request);
      if (errn) {
        DEBUG_MSG_ERROR(errn, common::logging::LOG_LEVEL_ERR);
      }
    }
  }
}

size_t InnerTcpHandlerHost::GetOnlineUserByStreamID(common::libev::IoLoop* server, stream_id sid) const {
  size_t total = 0;
  std::vector<common::libev::IoClient*> online_clients = server->GetClients();
  for (size_t i = 0; i < online_clients.size(); ++i) {
    common::libev::IoClient* client = online_clients[i];
    InnerTcpClient* iclient = static_cast<InnerTcpClient*>(client);
    if (iclient && iclient->GetCurrentStreamID() == sid) {
      total++;
    }
  }

  return total;
}

common::ErrnoError InnerTcpHandlerHost::HandleRequestClientActivate(InnerTcpClient* client, protocol::request_t* req) {
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jauth = json_tokener_parse(params_ptr);
    if (!jauth) {
      return common::make_errno_error_inval();
    }

    AuthInfo uauth;
    common::Error err_des = uauth.DeSerialize(jauth);
    json_object_put(jauth);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      protocol::response_t resp = ActivateResponseFail(req->id, err_str);
      client->WriteResponce(resp);
      return common::make_errno_error(err_str, EAGAIN);
    }

    if (!uauth.IsValid()) {
      return common::make_errno_error(EAGAIN);
    }

    UserInfo registered_user;
    common::Error err_find = parent_->FindUser(uauth, &registered_user);
    if (err_find) {
      return common::make_errno_error(EAGAIN);
    }

    const device_id_t dev = uauth.GetDeviceID();
    if (!registered_user.HaveDevice(dev)) {
      const std::string error_str = "Unknown device reject";
      protocol::response_t resp = ActivateResponseFail(req->id, error_str);
      client->WriteResponce(resp);
      return common::make_errno_error(error_str, EINVAL);
    }

    if (registered_user.IsBanned()) {
      const std::string error_str = "Banned user";
      protocol::response_t resp = ActivateResponseFail(req->id, error_str);
      client->WriteResponce(resp);
      return common::make_errno_error(error_str, EINVAL);
    }

    const ServerAuthInfo server_user_auth(registered_user.GetUserID(), uauth);
    if (server_user_auth == InnerTcpClient::anonim_user) {  // anonim user
      const protocol::response_t resp = ActivateResponseSuccess(req->id);
      common::ErrnoError err = client->WriteResponce(resp);
      if (err) {
        return err;
      }

      client->SetServerHostInfo(server_user_auth);
      INFO_LOG() << "Welcome anonim user: " << uauth.GetLogin();
      return common::ErrnoError();
    }

    // registered user
    const rpc::UserRpcInfo user_rpc = server_user_auth.MakeUserRpc();
    InnerTcpClient* fclient = parent_->FindInnerConnectionByUser(user_rpc);
    if (fclient) {
      const std::string error_str = "Double connection reject";
      protocol::response_t resp = ActivateResponseFail(req->id, error_str);
      client->WriteResponce(resp);
      return common::make_errno_error(error_str, EINVAL);
    }

    const protocol::response_t resp = ActivateResponseSuccess(req->id);
    common::ErrnoError errn = client->WriteResponce(resp);
    if (errn) {
      return errn;
    }

    common::Error err = parent_->RegisterInnerConnectionByUser(server_user_auth, client);
    CHECK(!err) << "Register inner connection error: " << err->GetDescription();

    PublishUserStateInfo(user_rpc, true);
    INFO_LOG() << "Welcome registered user: " << uauth.GetLogin();
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

common::ErrnoError InnerTcpHandlerHost::HandleRequestClientPing(InnerTcpClient* client, protocol::request_t* req) {
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jstop = json_tokener_parse(params_ptr);
    if (!jstop) {
      return common::make_errno_error_inval();
    }

    ClientPingInfo ping_info;
    common::Error err_des = ping_info.DeSerialize(jstop);
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

common::ErrnoError InnerTcpHandlerHost::HandleRequestClientGetServerInfo(InnerTcpClient* client,
                                                                         protocol::request_t* req) {
  AuthInfo hinf = client->GetServerHostInfo();
  UserInfo user;
  common::Error err = parent_->FindUser(hinf, &user);
  if (err) {
    const protocol::response_t resp = GetServerInfoResponceFail(req->id, err->GetDescription());
    ignore_result(client->WriteResponce(resp));
    ignore_result(client->Close());
    delete client;
    const std::string err_str = err->GetDescription();
    return common::make_errno_error(err_str, EAGAIN);
  }

  ServerInfo serv(config_.server.bandwidth_host);
  std::string server_info_str;
  common::Error err_ser = serv.SerializeToString(&server_info_str);
  if (err_ser) {
    const std::string err_str = err_ser->GetDescription();
    return common::make_errno_error(err_str, EAGAIN);
  }

  const protocol::response_t server_info_responce = GetServerInfoResponceSuccsess(req->id, server_info_str);
  return client->WriteResponce(server_info_responce);
}

common::ErrnoError InnerTcpHandlerHost::HandleRequestClientGetChannels(InnerTcpClient* client,
                                                                       protocol::request_t* req) {
  AuthInfo hinf = client->GetServerHostInfo();
  UserInfo user;
  common::Error err = parent_->FindUser(hinf, &user);
  if (err) {
    const std::string err_str = err->GetDescription();
    const protocol::response_t resp = GetServerInfoResponceFail(req->id, err_str);
    ignore_result(client->WriteResponce(resp));
    ignore_result(client->Close());
    delete client;
    return common::make_errno_error(err_str, EAGAIN);
  }

  std::string channels_str;
  ChannelsInfo chan = user.GetChannelInfo();
  common::Error err_ser = chan.SerializeToString(&channels_str);
  if (err_ser) {
    const std::string err_str = err_ser->GetDescription();
    return common::make_errno_error(err_str, EAGAIN);
  }

  const protocol::response_t channels_responce = GetChannelsResponceSuccsess(req->id, channels_str);
  return client->WriteResponce(channels_responce);
}

common::ErrnoError InnerTcpHandlerHost::HandleRequestClientGetRuntimeChannelInfo(InnerTcpClient* client,
                                                                                 protocol::request_t* req) {
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jrun = json_tokener_parse(params_ptr);
    if (!jrun) {
      return common::make_errno_error_inval();
    }

    RuntimeChannelLiteInfo run;
    common::Error err_des = run.DeSerialize(jrun);
    json_object_put(jrun);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    common::libev::IoLoop* server = client->GetServer();
    bool is_anonim = client->IsAnonimUser();
    AuthInfo ainf = client->GetServerHostInfo();
    const login_t login = ainf.GetLogin();
    const stream_id channel = run.GetChannelID();
    const stream_id prev_channel = client->GetCurrentStreamID();

    size_t watchers = GetOnlineUserByStreamID(server, channel);  // calc watchers
    client->SetCurrentStreamID(channel);                         // add to watcher

    RuntimeChannelInfo rinf;
    rinf.SetChannelID(channel);
    rinf.SetWatchersCount(watchers);
    if (!is_anonim) {  // registered user
      rinf.SetChatEnabled(false);
      rinf.SetChatReadOnly(true);
      rinf.SetChannelType(PRIVATE_CHANNEL);

      for (size_t i = 0; i < chat_channels_.size(); ++i) {
        if (chat_channels_[i] == channel) {
          rinf.SetChatEnabled(true);
          rinf.SetChatReadOnly(false);
          rinf.SetChannelType(OFFICAL_CHANNEL);
          break;
        }
      }
    } else {  // anonim have only offical channels and readonly mode
      rinf.SetChannelType(OFFICAL_CHANNEL);
      rinf.SetChatEnabled(true);
      rinf.SetChatReadOnly(true);
    }

    std::string rchannel_str;
    common::Error err_ser = rinf.SerializeToString(&rchannel_str);
    if (err_ser) {
      const std::string err_str = err_ser->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    const protocol::response_t channels_responce = GetRuntimeChannelInfoResponceSuccsess(req->id, rchannel_str);
    common::ErrnoError err = client->WriteResponce(channels_responce);
    if (err) {
      return err;
    }

    if (prev_channel == invalid_stream_id) {  // first channel
      SendEnterChatMessage(server, channel, login);
    } else {
      SendLeaveChatMessage(server, prev_channel, login);
      SendEnterChatMessage(server, channel, login);
    }
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

common::ErrnoError InnerTcpHandlerHost::HandleRequestClientSendChatMessage(InnerTcpClient* client,
                                                                           protocol::request_t* req) {
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jmsg = json_tokener_parse(params_ptr);
    if (!jmsg) {
      return common::make_errno_error_inval();
    }

    ChatMessage msg;
    common::Error err_des = msg.DeSerialize(jmsg);
    json_object_put(jmsg);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    BrodcastChatMessage(client->GetServer(), msg);
    const protocol::response_t resp = SendChatMessageResponceSuccsess(req->id, req->params);
    return client->WriteResponce(resp);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError InnerTcpHandlerHost::HandleRequestCommand(fastotv::inner::InnerClient* client,
                                                             protocol::request_t* req) {
  InnerTcpClient* iclient = static_cast<InnerTcpClient*>(client);
  if (req->method == CLIENT_ACTIVATE) {
    return HandleRequestClientActivate(iclient, req);
  } else if (req->method == CLIENT_PING) {
    return HandleRequestClientPing(iclient, req);
  } else if (req->method == CLIENT_GET_SERVER_INFO) {
    return HandleRequestClientGetServerInfo(iclient, req);
  } else if (req->method == CLIENT_GET_CHANNELS) {
    return HandleRequestClientGetChannels(iclient, req);
  } else if (req->method == CLIENT_GET_RUNTIME_CHANNEL_INFO) {
    return HandleRequestClientGetRuntimeChannelInfo(iclient, req);
  } else if (req->method == CLIENT_SEND_CHAT_MESSAGE) {
    return HandleRequestClientSendChatMessage(iclient, req);
  }

  WARNING_LOG() << "Received unknown command: " << req->method;
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandlerHost::HandleResponceServerPing(InnerTcpClient* client, protocol::response_t* resp) {
  UNUSED(client);
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jclient_ping = json_tokener_parse(params_ptr);
    if (!jclient_ping) {
      return common::make_errno_error_inval();
    }

    ServerPingInfo server_ping_info;
    common::Error err_des = server_ping_info.DeSerialize(jclient_ping);
    json_object_put(jclient_ping);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }
    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandlerHost::HandleResponceServerGetClientInfo(InnerTcpClient* client,
                                                                          protocol::response_t* resp) {
  UNUSED(client);
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jclient_info = json_tokener_parse(params_ptr);
    if (!jclient_info) {
      return common::make_errno_error_inval();
    }

    ClientInfo cinf;
    common::Error err_des = cinf.DeSerialize(jclient_info);
    json_object_put(jclient_info);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }
    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandlerHost::HandleResponceServerSendChatMessage(InnerTcpClient* client,
                                                                            protocol::response_t* resp) {
  UNUSED(client);
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jmsg_info = json_tokener_parse(params_ptr);
    if (!jmsg_info) {
      return common::make_errno_error_inval();
    }

    ChatMessage msg;
    common::Error err_des = msg.DeSerialize(jmsg_info);
    json_object_put(jmsg_info);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }
    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError InnerTcpHandlerHost::HandleResponceCommand(fastotv::inner::InnerClient* client,
                                                              protocol::response_t* resp) {
  protocol::request_t req;
  InnerTcpClient* sclient = static_cast<InnerTcpClient*>(client);
  InnerTcpClient::callback_t cb;
  if (sclient->PopRequestByID(resp->id, &req, &cb)) {
    if (cb) {
      cb(resp);
    }
    if (req.method == SERVER_PING) {
      return HandleResponceServerPing(sclient, resp);
    } else if (req.method == SERVER_GET_CLIENT_INFO) {
      return HandleResponceServerGetClientInfo(sclient, resp);
    } else if (req.method == SERVER_SEND_CHAT_MESSAGE) {
      return HandleResponceServerSendChatMessage(sclient, resp);
    } else {
      WARNING_LOG() << "HandleResponceServiceCommand not handled command: " << req.method;
    }
  }

  return common::ErrnoError();
}

}  // namespace inner
}  // namespace server
}  // namespace fastotv
