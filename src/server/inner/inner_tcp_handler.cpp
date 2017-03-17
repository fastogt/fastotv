/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of SiteOnYourDevice.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
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

void InnerTcpHandlerHost::PreLooped(network::tcp::ITcpLoop* server) {
  ping_client_id_timer_ = server->CreateTimer(ping_timeout_clients, ping_timeout_clients);
}

void InnerTcpHandlerHost::Moved(network::tcp::TcpClient* client) {
  UNUSED(client);
}

void InnerTcpHandlerHost::PostLooped(network::tcp::ITcpLoop* server) {
  UNUSED(server);
}

void InnerTcpHandlerHost::TimerEmited(network::tcp::ITcpLoop* server, network::timer_id_t id) {
  if (ping_client_id_timer_ == id) {
    std::vector<network::tcp::TcpClient*> online_clients = server->Clients();
    for (size_t i = 0; i < online_clients.size(); ++i) {
      network::tcp::TcpClient* client = online_clients[i];
      const cmd_request_t ping_request = PingRequest(NextId());
      ssize_t nwrite = 0;
      common::Error err = client->Write(ping_request.data(), ping_request.size(), &nwrite);
      INFO_LOG() << "Pinged sended " << nwrite << " byte, client[" << client->FormatedName()
                 << "], from server[" << server->FormatedName() << "], " << online_clients.size()
                 << " client(s) connected.";
      if (err && err->isError()) {
        DEBUG_MSG_ERROR(err);
        client->Close();
        delete client;
      }
    }
  }
}

void InnerTcpHandlerHost::Accepted(network::tcp::TcpClient* client) {
  ssize_t nwrite = 0;
  cmd_request_t whoareyou = WhoAreYouRequest(NextId());
  common::Error err = client->Write(whoareyou.data(), whoareyou.size(), &nwrite);
  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
  }
}

void InnerTcpHandlerHost::Closed(network::tcp::TcpClient* client) {
  bool is_ok = parent_->UnRegisterInnerConnectionByHost(client);
  if (is_ok) {
    InnerTcpClient* iconnection = static_cast<InnerTcpClient*>(client);
    if (iconnection) {
      AuthInfo hinf = iconnection->ServerHostInfo();
      std::string login = hinf.login;

      PublishStateToChannel(login, false);
    }
  }
}

void InnerTcpHandlerHost::DataReceived(network::tcp::TcpClient* client) {
  char buff[MAX_COMMAND_SIZE] = {0};
  ssize_t nread = 0;
  common::Error err = client->Read(buff, MAX_COMMAND_SIZE, &nread);
  if ((err && err->isError()) || nread == 0) {
    client->Close();
    delete client;
    return;
  }

  InnerTcpClient* iclient = static_cast<InnerTcpClient*>(client);
  HandleInnerDataReceived(iclient, buff, nread);
}

void InnerTcpHandlerHost::DataReadyToWrite(network::tcp::TcpClient* client) {
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

void InnerTcpHandlerHost::PublishStateToChannel(const std::string& login, bool connected) {
  std::string connected_resp;
  if (!connected) {
    connected_resp = common::MemSPrintf(SERVER_NOTIFY_CLIENT_DISCONNECTED_1S, login);
  } else {
    connected_resp = common::MemSPrintf(SERVER_NOTIFY_CLIENT_CONNECTED_1S, login);
  }
  bool res = sub_commands_in_->PublishStateToChannel(connected_resp);
  if (!res) {
    WARNING_LOG() << "Publish message: " << connected_resp << " to channel clients state failed.";
  }
}

inner::InnerTcpClient* InnerTcpHandlerHost::FindInnerConnectionByLogin(
    const std::string& login) const {
  return parent_->FindInnerConnectionByLogin(login);
}

void InnerTcpHandlerHost::HandleInnerRequestCommand(fastotv::inner::InnerClient* connection,
                                                    cmd_seq_t id,
                                                    int argc,
                                                    char* argv[]) {
  UNUSED(argc);
  char* command = argv[0];
  ssize_t nwrite = 0;

  if (IS_EQUAL_COMMAND(command, CLIENT_PING_COMMAND)) {
    cmd_responce_t pong = PingResponceSuccsess(id);
    common::Error err = connection->Write(pong, &nwrite);
    if (err && err->isError()) {
      DEBUG_MSG_ERROR(err);
    }
  } else if (IS_EQUAL_COMMAND(command, CLIENT_GET_CHANNELS)) {
    inner::InnerTcpClient* client = static_cast<inner::InnerTcpClient*>(connection);
    AuthInfo hinf = client->ServerHostInfo();
    UserInfo user;
    common::Error err = parent_->FindUser(hinf, &user);
    if (err && err->isError()) {
      cmd_responce_t resp = GetChannelsResponceFail(id, err->description());
      common::Error err = connection->Write(resp, &nwrite);
      if (err && err->isError()) {
        DEBUG_MSG_ERROR(err);
      }
      connection->Close();
      delete connection;
      return;
    }

    json_object* jchannels = MakeJobjectFromChannels(user.channels);
    std::string channels_str = json_object_get_string(jchannels);
    json_object_put(jchannels);
    std::string hex_channels = common::HexEncode(channels_str, false);
    cmd_responce_t channels_responce = GetChannelsResponceSuccsess(id, hex_channels);
    err = connection->Write(channels_responce, &nwrite);
    if (err && err->isError()) {
      cmd_responce_t resp2 = GetChannelsResponceFail(id, err->description());
      err = connection->Write(resp2, &nwrite);
      if (err && err->isError()) {
        DEBUG_MSG_ERROR(err);
      }
    }
  } else {
    WARNING_LOG() << "UNKNOWN COMMAND: " << command;
  }
}

void InnerTcpHandlerHost::HandleInnerResponceCommand(fastotv::inner::InnerClient* connection,
                                                     cmd_seq_t id,
                                                     int argc,
                                                     char* argv[]) {
  ssize_t nwrite = 0;
  char* state_command = argv[0];

  if (IS_EQUAL_COMMAND(state_command, SUCCESS_COMMAND) && argc > 1) {
    char* command = argv[1];
    if (IS_EQUAL_COMMAND(command, SERVER_PING_COMMAND)) {
      if (argc > 2) {
        const char* pong = argv[2];
        if (!pong) {
          cmd_approve_t resp = PingApproveResponceFail(id, "Invalid input argument(s)");
          common::Error err = connection->Write(resp, &nwrite);
          if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
          }
          goto fail;
        }

        cmd_approve_t resp = PingApproveResponceSuccsess(id);
        common::Error err = connection->Write(resp, &nwrite);
        if (err && err->isError()) {
          goto fail;
        }
      } else {
        cmd_approve_t resp = PingApproveResponceFail(id, "Invalid input argument(s)");
        common::Error err = connection->Write(resp, &nwrite);
        if (err && err->isError()) {
          DEBUG_MSG_ERROR(err);
        }
      }
    } else if (IS_EQUAL_COMMAND(command, SERVER_WHO_ARE_YOU_COMMAND)) {
      if (argc > 2) {
        const char* uauthstr = argv[2];
        if (!uauthstr) {
          cmd_approve_t resp = WhoAreYouApproveResponceFail(id, "Invalid input argument(s)");
          common::Error err = connection->Write(resp, &nwrite);
          if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
          }
          goto fail;
        }

        common::buffer_t buff = common::HexDecode(uauthstr);
        std::string buff_str(buff.begin(), buff.end());
        json_object* obj = json_tokener_parse(buff_str.c_str());
        if (!obj) {
          cmd_approve_t resp = WhoAreYouApproveResponceFail(id, "Invalid input argument(s)");
          common::Error err = connection->Write(resp, &nwrite);
          if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
          }
          goto fail;
        }

        AuthInfo uauth = AuthInfo::MakeClass(obj);
        json_object_put(obj);
        if (!uauth.IsValid()) {
          cmd_approve_t resp = WhoAreYouApproveResponceFail(id, "Invalid input argument(s)");
          common::Error err = connection->Write(resp, &nwrite);
          if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
          }
          goto fail;
        }

        common::Error err = parent_->FindUserAuth(uauth);
        if (err && err->isError()) {
          cmd_approve_t resp = WhoAreYouApproveResponceFail(id, err->description());
          common::Error err = connection->Write(resp, &nwrite);
          if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
          }
          goto fail;
        }

        std::string login = uauth.login;
        InnerTcpClient* fclient = parent_->FindInnerConnectionByLogin(login);
        if (fclient) {
          cmd_approve_t resp = WhoAreYouApproveResponceFail(id, "Double connection reject");
          common::Error err = connection->Write(resp, &nwrite);
          if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
          }
          goto fail;
        }

        cmd_approve_t resp = WhoAreYouApproveResponceSuccsess(id);
        err = connection->Write(resp, &nwrite);
        if (err && err->isError()) {
          cmd_approve_t resp2 = WhoAreYouApproveResponceFail(id, err->description());
          common::Error err = connection->Write(resp2, &nwrite);
          if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
          }
          goto fail;
        }

        bool is_ok = parent_->RegisterInnerConnectionByUser(uauth, connection);
        if (is_ok) {
          PublishStateToChannel(login, true);
        }
      } else {
        cmd_approve_t resp = WhoAreYouApproveResponceFail(id, "Invalid input argument(s)");
        common::Error err = connection->Write(resp, &nwrite);
        if (err && err->isError()) {
          DEBUG_MSG_ERROR(err);
        }
      }
    } else if (IS_EQUAL_COMMAND(command, SERVER_PLEASE_SYSTEM_INFO_COMMAND)) {
    } else {
      WARNING_LOG() << "UNKNOWN RESPONCE COMMAND: " << command;
    }
  } else if (IS_EQUAL_COMMAND(state_command, FAIL_COMMAND) && argc > 1) {
  } else {
    WARNING_LOG() << "UNKNOWN STATE COMMAND: " << state_command;
  }

  return;

fail:
  connection->Close();
  delete connection;
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
