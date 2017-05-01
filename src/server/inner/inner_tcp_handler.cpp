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

#include "server/inner/inner_tcp_client.h"

#include "server/inner/inner_external_notifier.h"

#define SERVER_NOTIFY_CLIENT_CONNECTED_1S "%s connected"
#define SERVER_NOTIFY_CLIENT_DISCONNECTED_1S "%s disconnected"

namespace fasto {
namespace fastotv {
namespace server {
namespace inner {

InnerTcpHandlerHost::InnerTcpHandlerHost(ServerHost* parent)
    : parent_(parent),
      sub_commands_in_(NULL),
      handler_(NULL),
      ping_client_id_timer_(INVALID_TIMER_ID) {
  handler_ = new InnerSubHandler(this);
  sub_commands_in_ = new RedisSub(handler_);
  redis_subscribe_command_in_thread_ =
      THREAD_MANAGER()->CreateThread(&RedisSub::Listen, sub_commands_in_);
}

InnerTcpHandlerHost::~InnerTcpHandlerHost() {
  sub_commands_in_->Stop();
  redis_subscribe_command_in_thread_->Join();
  delete sub_commands_in_;
  delete handler_;
}

void InnerTcpHandlerHost::PreLooped(network::IoLoop* server) {
  ping_client_id_timer_ = server->CreateTimer(ping_timeout_clients, ping_timeout_clients);
}

void InnerTcpHandlerHost::Moved(network::IoClient* client) {
  UNUSED(client);
}

void InnerTcpHandlerHost::PostLooped(network::IoLoop* server) {
  UNUSED(server);
}

void InnerTcpHandlerHost::TimerEmited(network::IoLoop* server, network::timer_id_t id) {
  if (ping_client_id_timer_ == id) {
    std::vector<network::IoClient*> online_clients = server->Clients();
    for (size_t i = 0; i < online_clients.size(); ++i) {
      network::IoClient* client = online_clients[i];
      InnerTcpClient* iclient = static_cast<InnerTcpClient*>(client);
      if (iclient) {
        const cmd_request_t ping_request = PingRequest(NextRequestID());
        common::Error err = iclient->Write(ping_request);
        if (err && err->IsError()) {
          DEBUG_MSG_ERROR(err);
          client->Close(err);
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

void InnerTcpHandlerHost::Accepted(network::IoClient* client) {
  cmd_request_t whoareyou = WhoAreYouRequest(NextRequestID());
  InnerTcpClient* iclient = static_cast<InnerTcpClient*>(client);
  if (iclient) {
    common::Error err = iclient->Write(whoareyou);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
  }
}

void InnerTcpHandlerHost::Closed(network::IoClient* client, common::Error err) {
  UNUSED(err);
  common::Error unreg_err = parent_->UnRegisterInnerConnectionByHost(client);
  if (unreg_err && unreg_err->IsError()) {
    DNOTREACHED();
    return;
  }

  InnerTcpClient* iconnection = static_cast<InnerTcpClient*>(client);
  if (iconnection) {
    user_id_t uid = iconnection->GetUid();
    PublishStateToChannel(uid, false);
  }
}

void InnerTcpHandlerHost::DataReceived(network::IoClient* client) {
  std::string buff;
  InnerTcpClient* iclient = static_cast<InnerTcpClient*>(client);
  common::Error err = iclient->ReadCommand(&buff);
  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
    client->Close(err);
    delete client;
    return;
  }

  HandleInnerDataReceived(iclient, buff);
}

void InnerTcpHandlerHost::DataReadyToWrite(network::IoClient* client) {
  UNUSED(client);
}

void InnerTcpHandlerHost::SetStorageConfig(const redis_sub_configuration_t& config) {
  sub_commands_in_->SetConfig(config);
  bool result = redis_subscribe_command_in_thread_->Start();
  if (!result) {
    WARNING_LOG() << "Don't started listen thread for external commands.";
  }
}

bool InnerTcpHandlerHost::PublishToChannelOut(const std::string& msg) {
  return sub_commands_in_->PublishToChannelOut(msg);
}

void InnerTcpHandlerHost::PublishStateToChannel(user_id_t uid, bool connected) {
  std::string connected_resp;
  if (!connected) {
    connected_resp = common::MemSPrintf(SERVER_NOTIFY_CLIENT_DISCONNECTED_1S, uid);
  } else {
    connected_resp = common::MemSPrintf(SERVER_NOTIFY_CLIENT_CONNECTED_1S, uid);
  }
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
    cmd_responce_t pong = PingResponceSuccsess(id);
    common::Error err = connection->Write(pong);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
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
      connection->Close(err);
      delete connection;
      return;
    }

    json_object* jchannels = MakeJobjectFromChannels(user.channels);
    std::string channels_str = json_object_get_string(jchannels);
    json_object_put(jchannels);
    std::string hex_channels = common::HexEncode(channels_str, false);
    cmd_responce_t channels_responce = GetChannelsResponceSuccsess(id, hex_channels);
    err = connection->Write(channels_responce);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    }
  } else {
    WARNING_LOG() << "UNKNOWN COMMAND: " << command;
  }
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
      connection->Close(err);
      delete connection;
    }
  } else if (IS_EQUAL_COMMAND(state_command, FAIL_COMMAND) && argc > 1) {
    common::Error err = HandleInnerFailedResponceCommand(connection, id, argc, argv);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
      connection->Close(err);
      delete connection;
    }
  } else {
    const std::string error_str = common::MemSPrintf("UNKNOWN STATE COMMAND: %s", state_command);
    common::Error err = common::make_error_value(error_str, common::Value::E_ERROR);
    DEBUG_MSG_ERROR(err);
    connection->Close(err);
    delete connection;
  }
}

common::Error InnerTcpHandlerHost::HandleInnerSuccsessResponceCommand(
    fastotv::inner::InnerClient* connection,
    cmd_seq_t id,
    int argc,
    char* argv[]) {
  char* command = argv[1];
  if (IS_EQUAL_COMMAND(command, SERVER_PING_COMMAND)) {
    if (argc < 2) {
      const std::string error_str = "Invalid input argument(s)";
      cmd_approve_t resp = PingApproveResponceFail(id, error_str);
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return common::make_error_value(error_str, common::Value::E_ERROR);
    }

    const char* pong = argv[2];
    if (!pong) {
      const std::string error_str = "Invalid input argument(s)";
      cmd_approve_t resp = PingApproveResponceFail(id, error_str);
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return common::make_error_value(error_str, common::Value::E_ERROR);
    }

    cmd_approve_t resp = PingApproveResponceSuccsess(id);
    common::Error err = connection->Write(resp);
    if (err && err->IsError()) {
      return err;
    }
    return common::Error();
  } else if (IS_EQUAL_COMMAND(command, SERVER_WHO_ARE_YOU_COMMAND)) {
    if (argc < 2) {
      const std::string error_str = "Invalid input argument(s)";
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, error_str);
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return common::make_error_value(error_str, common::Value::E_ERROR);
    }

    const char* uauthstr = argv[2];
    if (!uauthstr) {
      const std::string error_str = "Invalid input argument(s)";
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, error_str);
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return common::make_error_value(error_str, common::Value::E_ERROR);
    }

    common::buffer_t buff = common::HexDecode(uauthstr);
    std::string buff_str(buff.begin(), buff.end());
    json_object* obj = json_tokener_parse(buff_str.c_str());
    if (!obj) {
      const std::string error_str = "Invalid input argument(s)";
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, "Invalid input argument(s)");
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return common::make_error_value(error_str, common::Value::E_ERROR);
    }

    AuthInfo uauth = AuthInfo::MakeClass(obj);
    json_object_put(obj);
    if (!uauth.IsValid()) {
      const std::string error_str = "Invalid input argument(s)";
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, error_str);
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return common::make_error_value(error_str, common::Value::E_ERROR);
    }

    user_id_t uid;
    common::Error err = parent_->FindUserAuth(uauth, &uid);
    if (err && err->IsError()) {
      cmd_approve_t resp = WhoAreYouApproveResponceFail(id, err->Description());
      common::Error write_err = connection->Write(resp);
      UNUSED(write_err);
      return err;
    }

    std::string login = uauth.login;
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

    PublishStateToChannel(uid, true);
    return common::Error();
  } else if (IS_EQUAL_COMMAND(command, SERVER_PLEASE_SYSTEM_INFO_COMMAND)) {
    cmd_approve_t resp = SystemInfoApproveResponceSuccsess(id);
    common::Error err = connection->Write(resp);
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
      } else if (IS_EQUAL_COMMAND(okrespcommand, SERVER_WHO_ARE_YOU_COMMAND)) {
      }
    }
  } else if (IS_EQUAL_COMMAND(command, FAIL_COMMAND)) {
  } else {
    WARNING_LOG() << "UNKNOWN COMMAND: " << command;
  }
}

}  // namespace inner
}  // namespace server
}  // namespace fastotv
}  // namespace fasto
