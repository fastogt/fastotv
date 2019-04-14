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

#include <json-c/json_object.h>  // for json_object

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
#include "server/user_info.h"           // for user_id_t, UserInfo
#include "server/user_state_info.h"     // for UserStateInfo

namespace fastotv {
namespace server {
namespace inner {

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
          INFO_LOG() << "Pinged to client[" << client->GetFormatedName() << "], from server["
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
  InnerTcpClient* iconnection = static_cast<InnerTcpClient*>(client);
  AuthInfo auth = iconnection->GetServerHostInfo();
  common::libev::IoLoop* server = client->GetServer();
  SendLeaveChatMessage(server, iconnection->GetCurrentStreamId(), auth.GetLogin());

  if (iconnection->IsAnonimUser()) {  // anonim user
    INFO_LOG() << "Byu anonim user: " << auth.GetLogin();
    return;
  }

  common::Error unreg_err = parent_->UnRegisterInnerConnectionByHost(client);
  if (unreg_err) {
    DNOTREACHED();
    return;
  }

  user_id_t uid = iconnection->GetUid();
  PublishUserStateInfo(UserStateInfo(uid, auth.GetDeviceID(), false));
  INFO_LOG() << "Byu registered user: " << auth.GetLogin();
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

void InnerTcpHandlerHost::PublishUserStateInfo(const UserStateInfo& state) {
  json_object* user_state_json = nullptr;
  common::Error err = state.Serialize(&user_state_json);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    return;
  }

  std::string connected_resp = json_object_get_string(user_state_json);
  json_object_put(user_state_json);
  err = sub_commands_in_->PublishStateToChannel(connected_resp);
  if (err) {
    WARNING_LOG() << "Publish message: " << connected_resp << " to channel clients state failed.";
  }
}

inner::InnerTcpClient* InnerTcpHandlerHost::FindInnerConnectionByUserIDAndDeviceID(user_id_t user,
                                                                                   device_id_t dev) const {
  return parent_->FindInnerConnectionByUserIDAndDeviceID(user, dev);
}

void InnerTcpHandlerHost::SendEnterChatMessage(common::libev::IoLoop* server, stream_id sid, login_t login) {
  BrodcastChatMessage(server, MakeEnterMessage(sid, login));
}

void InnerTcpHandlerHost::SendLeaveChatMessage(common::libev::IoLoop* server, stream_id sid, login_t login) {
  BrodcastChatMessage(server, MakeLeaveMessage(sid, login));
}

void InnerTcpHandlerHost::BrodcastChatMessage(common::libev::IoLoop* server, const ChatMessage& msg) {
  serializet_t msg_ser;
  common::Error err = msg.SerializeToString(&msg_ser);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    return;
  }

  std::vector<common::libev::IoClient*> online_clients = server->GetClients();
  for (size_t i = 0; i < online_clients.size(); ++i) {
    common::libev::IoClient* client = online_clients[i];
    InnerTcpClient* iclient = static_cast<InnerTcpClient*>(client);
    if (iclient && iclient->GetCurrentStreamId() == msg.GetChannelId()) {
      const protocol::request_t message_request = ServerSendChatMessageRequest(NextRequestID(), msg_ser);
      common::ErrnoError errn = iclient->WriteRequest(message_request);
      if (errn) {
        DEBUG_MSG_ERROR(errn, common::logging::LOG_LEVEL_ERR);
      }
    }
  }
}

size_t InnerTcpHandlerHost::GetOnlineUserByStreamId(common::libev::IoLoop* server, stream_id sid) const {
  size_t total = 0;
  std::vector<common::libev::IoClient*> online_clients = server->GetClients();
  for (size_t i = 0; i < online_clients.size(); ++i) {
    common::libev::IoClient* client = online_clients[i];
    InnerTcpClient* iclient = static_cast<InnerTcpClient*>(client);
    if (iclient && iclient->GetCurrentStreamId() == sid) {
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

    user_id_t uid;
    UserInfo registered_user;
    common::Error err_find = parent_->FindUser(uauth, &uid, &registered_user);
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

    if (uauth == InnerTcpClient::anonim_user) {  // anonim user
      const protocol::response_t resp = ActivateResponseSuccess(req->id);
      common::ErrnoError err = client->WriteResponce(resp);
      if (err) {
        return err;
      }

      client->SetServerHostInfo(uauth);
      INFO_LOG() << "Welcome anonim user: " << uauth.GetLogin();
      return common::ErrnoError();
    }

    // registered user
    InnerTcpClient* fclient = parent_->FindInnerConnectionByUserIDAndDeviceID(uid, dev);
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

    common::Error err = parent_->RegisterInnerConnectionByUser(uid, uauth, client);
    CHECK(!err) << "Register inner connection error: " << err->GetDescription();

    PublishUserStateInfo(UserStateInfo(uid, dev, true));
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
  user_id_t uid;
  common::Error err_ser = parent_->FindUser(hinf, &uid, &user);
  if (err_ser) {
    const protocol::response_t resp = GetServerInfoResponceFail(req->id, err_ser->GetDescription());
    ignore_result(client->WriteResponce(resp));
    ignore_result(client->Close());
    delete client;
    const std::string err_str = err_ser->GetDescription();
    return common::make_errno_error(err_str, EAGAIN);
  }

  ServerInfo serv(config_.server.bandwidth_host);
  std::string server_info_str;
  err_ser = serv.SerializeToString(&server_info_str);
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
  user_id_t uid;
  common::Error err = parent_->FindUser(hinf, &uid, &user);
  if (err) {
    const std::string err_str = err->GetDescription();
    const protocol::response_t resp = GetServerInfoResponceFail(req->id, err_str);
    ignore_result(client->WriteResponce(resp));
    ignore_result(client->Close());
    delete client;
    return common::make_errno_error(err_str, EAGAIN);
  }

  serializet_t channels_str;
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
    const stream_id channel = run.GetChannelId();
    const stream_id prev_channel = client->GetCurrentStreamId();

    size_t watchers = GetOnlineUserByStreamId(server, channel);  // calc watchers
    client->SetCurrentStreamId(channel);                         // add to watcher

    RuntimeChannelInfo rinf;
    rinf.SetChannelId(channel);
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

    serializet_t rchannel_str;
    common::Error err_ser = rinf.SerializeToString(&rchannel_str);
    if (err_ser) {
      const std::string err_str = err_ser->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    const protocol::response_t channels_responce = GetRuntimeChannelInfoResponceSuccsess(req->id, rchannel_str);
    common::ErrnoError err = client->WriteResponce(channels_responce);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    } else {
      if (prev_channel == invalid_stream_id) {  // first channel
        SendEnterChatMessage(server, channel, login);
      } else {
        SendLeaveChatMessage(server, prev_channel, login);
        SendEnterChatMessage(server, channel, login);
      }
    }

    return err;
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
  if (sclient->PopRequestByID(resp->id, &req)) {
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
