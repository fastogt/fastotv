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

#include <string>
#include <vector>

#include "third-party/json-c/json-c/json.h"  // for json_object_...

#undef ERROR
#include <common/net/net.h>
#include <common/logger.h>
#include <common/threads/thread_manager.h>
#include <common/convert2string.h>

#include "server/commands.h"
#include "server/server_host.h"

#include "server_info.h"
#include "client_info.h"
#include "ping_info.h"

#include "server/inner/inner_tcp_client.h"

#include "server/inner/inner_external_notifier.h"

namespace fasto {
namespace fastotv {
namespace server {
namespace inner {

InnerTcpHandlerHost::InnerTcpHandlerHost(ServerHost* parent, const Config& config)
    : parent_(parent),
      sub_commands_in_(NULL),
      handler_(NULL),
      ping_client_id_timer_(INVALID_TIMER_ID),
      config_(config) {
  handler_ = new InnerSubHandler(this);
  sub_commands_in_ = new RedisSub(handler_);
  redis_subscribe_command_in_thread_ =
      THREAD_MANAGER()->CreateThread(&RedisSub::Listen, sub_commands_in_);

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
  ping_client_id_timer_ = server->CreateTimer(ping_timeout_clients, ping_timeout_clients);
}

void InnerTcpHandlerHost::Moved(common::libev::IoLoop* server, common::libev::IoClient* client) {
  UNUSED(server);
  UNUSED(client);
}

void InnerTcpHandlerHost::PostLooped(common::libev::IoLoop* server) {
  UNUSED(server);
}

void InnerTcpHandlerHost::TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) {
  if (ping_client_id_timer_ == id) {
    std::vector<common::libev::IoClient*> online_clients = server->Clients();
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
          INFO_LOG() << "Pinged to client[" << client->FormatedName() << "], from server["
                     << server->FormatedName() << "], " << online_clients.size()
                     << " client(s) connected.";
        }
      }
    }
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
  common::Error unreg_err = parent_->UnRegisterInnerConnectionByHost(client);
  if (unreg_err && unreg_err->IsError()) {
    DNOTREACHED();
    return;
  }

  InnerTcpClient* iconnection = static_cast<InnerTcpClient*>(client);
  if (iconnection) {
    user_id_t uid = iconnection->GetUid();
    PublishUserStateInfo(UserStateInfo(uid, false));
  }
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

bool InnerTcpHandlerHost::PublishToChannelOut(const std::string& msg) {
  return sub_commands_in_->PublishToChannelOut(msg);
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
  bool res = sub_commands_in_->PublishStateToChannel(connected_resp);
  if (!res) {
    WARNING_LOG() << "Publish message: " << connected_resp << " to channel clients state failed.";
  }
}

inner::InnerTcpClient* InnerTcpHandlerHost::FindInnerConnectionByID(
    const std::string& login) const {
  return parent_->FindInnerConnectionByID(login);
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
      cmd_responce_t resp = PingResponceFail(id, err->Description());
      common::Error err = connection->Write(resp);
      if (err && err->IsError()) {
        DEBUG_MSG_ERROR(err);
      }
      connection->Close();
      delete connection;
      return;
    }
    std::string ping_info_str = json_object_get_string(jping_info);
    json_object_put(jping_info);
    std::string ping_info = Encode(ping_info_str);

    cmd_responce_t pong = PingResponceSuccsess(id, ping_info);
    err = connection->Write(pong);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
    return;
  } else if (IS_EQUAL_COMMAND(command, CLIENT_GET_SERVER_INFO)) {
    inner::InnerTcpClient* client = static_cast<inner::InnerTcpClient*>(connection);
    AuthInfo hinf = client->ServerHostInfo();
    UserInfo user;
    user_id_t uid;
    common::Error err = parent_->FindUser(hinf, &uid, &user);
    if (err && err->IsError()) {
      cmd_responce_t resp = GetServerInfoResponceFail(id, err->Description());
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

    std::string server_info_str = json_object_get_string(jserver_info);
    json_object_put(jserver_info);
    std::string hex_server_info = Encode(server_info_str);

    cmd_responce_t server_info_responce = GetServerInfoResponceSuccsess(id, hex_server_info);
    err = connection->Write(server_info_responce);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
    return;
  } else if (IS_EQUAL_COMMAND(command, CLIENT_GET_CHANNELS)) {
    inner::InnerTcpClient* client = static_cast<inner::InnerTcpClient*>(connection);
    AuthInfo hinf = client->ServerHostInfo();
    UserInfo user;
    user_id_t uid;
    common::Error err = parent_->FindUser(hinf, &uid, &user);
    if (err && err->IsError()) {
      cmd_responce_t resp = GetChannelsResponceFail(id, err->Description());
      common::Error err = connection->Write(resp);
      if (err && err->IsError()) {
        DEBUG_MSG_ERROR(err);
      }
      connection->Close();
      delete connection;
      return;
    }

    std::string channels_str;
    ChannelsInfo chan = user.GetChannelInfo();
    err = chan.SerializeToString(&channels_str);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
      return;
    }

    std::string enc_channels = Encode(channels_str);
    cmd_responce_t channels_responce = GetChannelsResponceSuccsess(id, enc_channels);
    err = connection->Write(channels_responce);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
    return;
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

common::Error InnerTcpHandlerHost::HandleInnerSuccsessResponceCommand(
    fastotv::inner::InnerClient* connection,
    cmd_seq_t id,
    int argc,
    char* argv[]) {
  char* command = argv[1];
  if (IS_EQUAL_COMMAND(command, SERVER_PING_COMMAND)) {
    json_object* obj = NULL;
    common::Error parse_err = ParserResponceResponceCommand(argc, argv, &obj);
    if (parse_err && parse_err->IsError()) {
      cmd_approve_t resp = PingApproveResponceFail(id, parse_err->Description());
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return parse_err;
    }

    ServerPingInfo ping_info;
    common::Error err = ServerPingInfo::DeSerialize(obj, &ping_info);
    json_object_put(obj);
    if (err && err->IsError()) {
      cmd_approve_t resp = PingApproveResponceFail(id, parse_err->Description());
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
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, parse_err->Description());
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return parse_err;
    }

    AuthInfo uauth;
    common::Error err = AuthInfo::DeSerialize(obj, &uauth);
    json_object_put(obj);
    if (err && err->IsError()) {
      const std::string error_str = err->Description();
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, error_str);
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return err;
    }

    if (!uauth.IsValid()) {
      const std::string error_str = "Invalid input argument(s)";
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, error_str);
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return common::make_error_value(error_str, common::Value::E_ERROR);
    }

    user_id_t uid;
    err = parent_->FindUserAuth(uauth, &uid);
    if (err && err->IsError()) {
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, err->Description());
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return err;
    }

    std::string login = uauth.GetLogin();
    InnerTcpClient* fclient = parent_->FindInnerConnectionByID(login);
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

    PublishUserStateInfo(UserStateInfo(uid, true));
    return common::Error();
  } else if (IS_EQUAL_COMMAND(command, SERVER_GET_CLIENT_INFO_COMMAND)) {  // encoded
    json_object* obj = NULL;
    common::Error parse_err = ParserResponceResponceCommand(argc, argv, &obj);
    if (parse_err && parse_err->IsError()) {
      cmd_approve_t resp = SystemInfoApproveResponceFail(id, parse_err->Description());
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return parse_err;
    }

    ClientInfo cinf;
    common::Error err = ClientInfo::DeSerialize(obj, &cinf);
    json_object_put(obj);
    if (err && err->IsError()) {
      const std::string error_str = err->Description();
      cmd_approve_t resp = SystemInfoApproveResponceFail(id, error_str);
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return err;
    }

    if (!cinf.IsValid()) {
      const std::string error_str = "Invalid input argument(s)";
      cmd_approve_t resp = SystemInfoApproveResponceFail(id, error_str);
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return common::make_error_value(error_str, common::Value::E_ERROR);
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

common::Error InnerTcpHandlerHost::HandleInnerFailedResponceCommand(
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
      }
    }
    return;
  } else if (IS_EQUAL_COMMAND(command, FAIL_COMMAND)) {
    if (argc > 1) {
      const char* failed_resp_command = argv[1];
      if (IS_EQUAL_COMMAND(failed_resp_command, CLIENT_PING_COMMAND)) {
      } else if (IS_EQUAL_COMMAND(failed_resp_command, CLIENT_GET_SERVER_INFO)) {
      } else if (IS_EQUAL_COMMAND(failed_resp_command, CLIENT_GET_CHANNELS)) {
      }
    }
    return;
  }

  WARNING_LOG() << "UNKNOWN COMMAND: " << command;
}

common::Error InnerTcpHandlerHost::ParserResponceResponceCommand(int argc,
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
}  // namespace server
}  // namespace fastotv
}  // namespace fasto
