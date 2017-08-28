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

#include "server/inner/inner_tcp_handler.h"

#include <stddef.h>  // for NULL
#include <string>    // for string

#include <json-c/json_object.h>  // for json_object

#include <common/libev/io_client.h>         // for IoClient
#include <common/libev/io_loop.h>           // for IoLoop
#include <common/logger.h>                  // for COMPACT_LOG_WARNING
#include <common/threads/thread_manager.h>  // for THREAD_MANAGER
#include <common/value.h>                   // for Value, Value::Erro...

#include "auth_info.h"            // for AuthInfo
#include "channels_info.h"        // for ChannelsInfo
#include "client_info.h"          // for ClientInfo
#include "client_server_types.h"  // for Encode
#include "inner/inner_client.h"   // for InnerClient
#include "ping_info.h"            // for ClientPingInfo

#include "server/commands.h"

#include "server/redis/redis_pub_sub.h"

#include "server/inner/inner_external_notifier.h"  // for InnerSubHandler
#include "server/inner/inner_tcp_client.h"         // for InnerTcpClient

#include "runtime_channel_info.h"
#include "server/server_host.h"      // for ServerHost
#include "server/user_info.h"        // for user_id_t, UserInfo
#include "server/user_state_info.h"  // for UserStateInfo
#include "server_info.h"             // for ServerInfo

namespace fastotv {
namespace server {
namespace inner {

InnerTcpHandlerHost::InnerTcpHandlerHost(ServerHost* parent, const Config& config)
    : parent_(parent),
      sub_commands_in_(NULL),
      handler_(NULL),
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
        const cmd_request_t ping_request = PingRequest(NextRequestID());
        common::Error err = iclient->Write(ping_request);
        if (err && err->IsError()) {
          DEBUG_MSG_ERROR(err);
          client->Close();
          delete client;
        } else {
          INFO_LOG() << "Pinged to client[" << client->FormatedName() << "], from server[" << server->FormatedName()
                     << "], " << online_clients.size() << " client(s) connected.";
        }
      }
    }
  } else if (reread_cache_id_timer_ == id) {
    UpdateCache();
  }
}

void InnerTcpHandlerHost::Accepted(common::libev::IoClient* client) {
  cmd_request_t whoareyou = WhoAreYouRequest(NextRequestID());
  InnerTcpClient* iclient = static_cast<InnerTcpClient*>(client);
  if (iclient) {
    common::Error err = iclient->Write(whoareyou);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
  }
}

void InnerTcpHandlerHost::Closed(common::libev::IoClient* client) {
  InnerTcpClient* iconnection = static_cast<InnerTcpClient*>(client);
  AuthInfo auth = iconnection->GetServerHostInfo();
  if (iconnection->IsAnonimUser()) {  // anonim user
    INFO_LOG() << "Byu anonim user: " << auth.GetLogin();
    return;
  }

  common::Error unreg_err = parent_->UnRegisterInnerConnectionByHost(client);
  if (unreg_err && unreg_err->IsError()) {
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
  common::Error err = iclient->ReadCommand(&buff);
  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
    client->Close();
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
  parent_->GetChatChannels(&channels);
  chat_channels_ = channels;
}

void InnerTcpHandlerHost::PublishUserStateInfo(const UserStateInfo& state) {
  json_object* user_state_json = NULL;
  common::Error err = state.Serialize(&user_state_json);
  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
    return;
  }

  std::string connected_resp = json_object_get_string(user_state_json);
  json_object_put(user_state_json);
  err = sub_commands_in_->PublishStateToChannel(connected_resp);
  if (err && err->IsError()) {
    WARNING_LOG() << "Publish message: " << connected_resp << " to channel clients state failed.";
  }
}

inner::InnerTcpClient* InnerTcpHandlerHost::FindInnerConnectionByUserIDAndDeviceID(user_id_t user,
                                                                                   device_id_t dev) const {
  return parent_->FindInnerConnectionByUserIDAndDeviceID(user, dev);
}

void InnerTcpHandlerHost::HandleInnerRequestCommand(fastotv::inner::InnerClient* connection,
                                                    cmd_seq_t id,
                                                    int argc,
                                                    char* argv[]) {
  UNUSED(argc);
  char* command = argv[0];
  if (IS_EQUAL_COMMAND(command, CLIENT_PING_COMMAND)) {
    ClientPingInfo ping;
    json_object* jping_info = NULL;
    common::Error err = ping.Serialize(&jping_info);
    if (err && err->IsError()) {
      cmd_responce_t resp = PingResponceFail(id, err->GetDescription());
      common::Error err = connection->Write(resp);
      if (err && err->IsError()) {
        DEBUG_MSG_ERROR(err);
      }
      connection->Close();
      delete connection;
      return;
    }
    serializet_t ping_info_str = json_object_get_string(jping_info);
    json_object_put(jping_info);

    cmd_responce_t pong = PingResponceSuccsess(id, ping_info_str);
    err = connection->Write(pong);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
    return;
  } else if (IS_EQUAL_COMMAND(command, CLIENT_GET_SERVER_INFO)) {
    inner::InnerTcpClient* client = static_cast<inner::InnerTcpClient*>(connection);
    AuthInfo hinf = client->GetServerHostInfo();
    UserInfo user;
    user_id_t uid;
    common::Error err = parent_->FindUser(hinf, &uid, &user);
    if (err && err->IsError()) {
      cmd_responce_t resp = GetServerInfoResponceFail(id, err->GetDescription());
      common::Error err = connection->Write(resp);
      if (err && err->IsError()) {
        DEBUG_MSG_ERROR(err);
      }
      connection->Close();
      delete connection;
      return;
    }

    ServerInfo serv(config_.server.bandwidth_host);
    json_object* jserver_info = NULL;
    err = serv.Serialize(&jserver_info);
    if (err && err->IsError()) {
      NOTREACHED();
    }

    serializet_t server_info_str = json_object_get_string(jserver_info);
    json_object_put(jserver_info);

    cmd_responce_t server_info_responce = GetServerInfoResponceSuccsess(id, server_info_str);
    err = connection->Write(server_info_responce);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
    return;
  } else if (IS_EQUAL_COMMAND(command, CLIENT_GET_CHANNELS)) {
    inner::InnerTcpClient* client = static_cast<inner::InnerTcpClient*>(connection);
    AuthInfo hinf = client->GetServerHostInfo();
    UserInfo user;
    user_id_t uid;
    common::Error err = parent_->FindUser(hinf, &uid, &user);
    if (err && err->IsError()) {
      cmd_responce_t resp = GetChannelsResponceFail(id, err->GetDescription());
      common::Error err = connection->Write(resp);
      if (err && err->IsError()) {
        DEBUG_MSG_ERROR(err);
      }
      connection->Close();
      delete connection;
      return;
    }

    serializet_t channels_str;
    ChannelsInfo chan = user.GetChannelInfo();
    err = chan.SerializeToString(&channels_str);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
      return;
    }

    cmd_responce_t channels_responce = GetChannelsResponceSuccsess(id, channels_str);
    err = connection->Write(channels_responce);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
    return;
  } else if (IS_EQUAL_COMMAND(command, CLIENT_GET_RUNTIME_CHANNEL_INFO)) {
    inner::InnerTcpClient* client = static_cast<inner::InnerTcpClient*>(connection);
    bool is_anonim = client->IsAnonimUser();
    if (argc > 1) {
      char* channel_id_str = argv[1];
      client->SetCurrentStreamId(channel_id_str);
      for (size_t i = 0; i < chat_channels_.size(); ++i) {
        if (chat_channels_[i] == channel_id_str) {
          serializet_t rchannel_str;
          RuntimeChannelInfo rinf(channel_id_str, GetOnlineUserByStreamId(client->GetServer(), channel_id_str),
                                  !is_anonim, std::vector<std::string>());
          common::Error err = rinf.SerializeToString(&rchannel_str);
          if (err && err->IsError()) {
            DEBUG_MSG_ERROR(err);
            return;
          }

          cmd_responce_t channels_responce = GetRuntimeChannelInfoResponceSuccsess(id, rchannel_str);
          err = connection->Write(channels_responce);
          if (err && err->IsError()) {
            DEBUG_MSG_ERROR(err);
          }
          return;
        }
      }

      const std::string error_str = "Not found chat room.";
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, error_str);
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return;
    }
  }

  WARNING_LOG() << "UNKNOWN COMMAND: " << command;
}

void InnerTcpHandlerHost::HandleInnerResponceCommand(fastotv::inner::InnerClient* connection,
                                                     cmd_seq_t id,
                                                     int argc,
                                                     char* argv[]) {
  char* state_command = argv[0];

  if (IS_EQUAL_COMMAND(state_command, SUCCESS_COMMAND) && argc > 1) {
    common::Error err = HandleInnerSuccsessResponceCommand(connection, id, argc, argv);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
      connection->Close();
      delete connection;
    }
    return;
  } else if (IS_EQUAL_COMMAND(state_command, FAIL_COMMAND) && argc > 1) {
    common::Error err = HandleInnerFailedResponceCommand(connection, id, argc, argv);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
      connection->Close();
      delete connection;
    }
    return;
  }

  const std::string error_str = common::MemSPrintf("UNKNOWN STATE COMMAND: %s", state_command);
  common::Error err = common::make_error_value(error_str, common::Value::E_ERROR);
  DEBUG_MSG_ERROR(err);
  connection->Close();
  delete connection;
}

common::Error InnerTcpHandlerHost::HandleInnerSuccsessResponceCommand(fastotv::inner::InnerClient* connection,
                                                                      cmd_seq_t id,
                                                                      int argc,
                                                                      char* argv[]) {
  char* command = argv[1];
  if (IS_EQUAL_COMMAND(command, SERVER_PING_COMMAND)) {
    json_object* obj = NULL;
    common::Error parse_err = ParserResponceResponceCommand(argc, argv, &obj);
    if (parse_err && parse_err->IsError()) {
      cmd_approve_t resp = PingApproveResponceFail(id, parse_err->GetDescription());
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return parse_err;
    }

    ServerPingInfo ping_info;
    common::Error err = ServerPingInfo::DeSerialize(obj, &ping_info);
    json_object_put(obj);
    if (err && err->IsError()) {
      cmd_approve_t resp = PingApproveResponceFail(id, parse_err->GetDescription());
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return err;
    }

    cmd_approve_t resp = PingApproveResponceSuccsess(id);
    err = connection->Write(resp);
    if (err && err->IsError()) {
      return err;
    }
    return common::Error();
  } else if (IS_EQUAL_COMMAND(command, SERVER_WHO_ARE_YOU_COMMAND)) {  // encoded
    json_object* obj = NULL;
    common::Error parse_err = ParserResponceResponceCommand(argc, argv, &obj);
    if (parse_err && parse_err->IsError()) {
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, parse_err->GetDescription());
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return parse_err;
    }

    AuthInfo uauth;
    common::Error err = AuthInfo::DeSerialize(obj, &uauth);
    json_object_put(obj);
    if (err && err->IsError()) {
      const std::string error_str = err->GetDescription();
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, error_str);
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return err;
    }

    if (!uauth.IsValid()) {
      common::Error lerr = common::make_inval_error_value(common::Value::E_ERROR);
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, lerr->GetDescription());
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return lerr;
    }

    user_id_t uid;
    UserInfo registered_user;
    err = parent_->FindUser(uauth, &uid, &registered_user);
    if (err && err->IsError()) {
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, err->GetDescription());
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return err;
    }

    const device_id_t dev = uauth.GetDeviceID();
    if (!registered_user.HaveDevice(dev)) {
      const std::string error_str = "Unknown device reject";
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, error_str);
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return err;
    }

    if (uauth == InnerTcpClient::anonim_user) {  // anonim user
      cmd_approve_t resp = WhoAreYouApproveResponceSuccsess(id);
      err = connection->Write(resp);
      if (err && err->IsError()) {
        return err;
      }

      InnerTcpClient* inner_conn = static_cast<InnerTcpClient*>(connection);
      inner_conn->SetServerHostInfo(uauth);
      INFO_LOG() << "Welcome anonim user: " << uauth.GetLogin();
      return common::Error();
    }

    // registered user
    InnerTcpClient* fclient = parent_->FindInnerConnectionByUserIDAndDeviceID(uid, dev);
    if (fclient) {
      const std::string error_str = "Double connection reject";
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, error_str);
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return err;
    }

    cmd_approve_t resp = WhoAreYouApproveResponceSuccsess(id);
    err = connection->Write(resp);
    if (err && err->IsError()) {
      return err;
    }

    err = parent_->RegisterInnerConnectionByUser(uid, uauth, connection);
    if (err && err->IsError()) {
      return err;
    }

    PublishUserStateInfo(UserStateInfo(uid, dev, true));
    INFO_LOG() << "Welcome registered user: " << uauth.GetLogin();
    return common::Error();
  } else if (IS_EQUAL_COMMAND(command, SERVER_GET_CLIENT_INFO_COMMAND)) {  // encoded
    json_object* obj = NULL;
    common::Error parse_err = ParserResponceResponceCommand(argc, argv, &obj);
    if (parse_err && parse_err->IsError()) {
      cmd_approve_t resp = SystemInfoApproveResponceFail(id, parse_err->GetDescription());
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return parse_err;
    }

    ClientInfo cinf;
    common::Error err = ClientInfo::DeSerialize(obj, &cinf);
    json_object_put(obj);
    if (err && err->IsError()) {
      const std::string error_str = err->GetDescription();
      cmd_approve_t resp = SystemInfoApproveResponceFail(id, error_str);
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return err;
    }

    if (!cinf.IsValid()) {
      common::Error lerr = common::make_inval_error_value(common::Value::E_ERROR);
      cmd_approve_t resp = SystemInfoApproveResponceFail(id, lerr->GetDescription());
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return lerr;
    }

    cmd_approve_t resp = SystemInfoApproveResponceSuccsess(id);
    err = connection->Write(resp);
    if (err && err->IsError()) {
      return err;
    }
    return common::Error();
  }

  const std::string error_str = common::MemSPrintf("UNKNOWN RESPONCE COMMAND: %s", command);
  return common::make_error_value(error_str, common::Value::E_ERROR);
}

common::Error InnerTcpHandlerHost::HandleInnerFailedResponceCommand(fastotv::inner::InnerClient* connection,
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

void InnerTcpHandlerHost::HandleInnerApproveCommand(fastotv::inner::InnerClient* connection,
                                                    cmd_seq_t id,
                                                    int argc,
                                                    char* argv[]) {
  UNUSED(connection);
  UNUSED(id);
  char* command = argv[0];

  if (IS_EQUAL_COMMAND(command, SUCCESS_COMMAND)) {
    if (argc > 1) {
      const char* okrespcommand = argv[1];
      if (IS_EQUAL_COMMAND(okrespcommand, CLIENT_PING_COMMAND)) {
      } else if (IS_EQUAL_COMMAND(okrespcommand, CLIENT_GET_SERVER_INFO)) {
      } else if (IS_EQUAL_COMMAND(okrespcommand, CLIENT_GET_CHANNELS)) {
      } else if (IS_EQUAL_COMMAND(okrespcommand, CLIENT_GET_RUNTIME_CHANNEL_INFO)) {
      }
    }
    return;
  } else if (IS_EQUAL_COMMAND(command, FAIL_COMMAND)) {
    if (argc > 1) {
      const char* failed_resp_command = argv[1];
      if (IS_EQUAL_COMMAND(failed_resp_command, CLIENT_PING_COMMAND)) {
      } else if (IS_EQUAL_COMMAND(failed_resp_command, CLIENT_GET_SERVER_INFO)) {
      } else if (IS_EQUAL_COMMAND(failed_resp_command, CLIENT_GET_CHANNELS)) {
      } else if (IS_EQUAL_COMMAND(failed_resp_command, CLIENT_GET_RUNTIME_CHANNEL_INFO)) {
      }
    }
    return;
  }

  WARNING_LOG() << "UNKNOWN COMMAND: " << command;
}

common::Error InnerTcpHandlerHost::ParserResponceResponceCommand(int argc, char* argv[], json_object** out) {
  if (argc < 2) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }

  const char* arg_2_str = argv[2];
  if (!arg_2_str) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }

  json_object* obj = json_tokener_parse(arg_2_str);
  if (!obj) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }

  *out = obj;
  return common::Error();
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

}  // namespace inner
}  // namespace server
}  // namespace fastotv
