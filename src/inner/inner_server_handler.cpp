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

#include "inner/inner_server_handler.h"

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

#include "client_commands.h"

#include "core/events/network_events.h"

namespace fasto {
namespace fastotv {
namespace inner {

InnerServerHandler::InnerServerHandler(const common::net::HostAndPort& innerHost,
                                       const TvConfig& config)
    : config_(config),
      inner_connection_(nullptr),
      ping_server_id_timer_(INVALID_TIMER_ID),
      innerHost_(innerHost) {}

InnerServerHandler::~InnerServerHandler() {
  delete inner_connection_;
  inner_connection_ = nullptr;
}

void InnerServerHandler::preLooped(tcp::ITcpLoop* server) {
  ping_server_id_timer_ = server->createTimer(ping_timeout_server, ping_timeout_server);
  CHECK(!inner_connection_);

  common::net::socket_info client_info;
  common::ErrnoError err =
      common::net::connect(innerHost_, common::net::ST_SOCK_STREAM, 0, &client_info);
  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
    auto ex_event =
        make_exception_event(new core::events::ClientConnectedEvent(this, authInfo()), err);
    fApp->PostEvent(ex_event);
    return;
  }

  InnerClient* connection = new InnerClient(server, client_info);
  inner_connection_ = connection;
  server->registerClient(connection);
}

void InnerServerHandler::accepted(tcp::TcpClient* client) {
  UNUSED(client);
}

void InnerServerHandler::moved(tcp::TcpClient* client) {
  UNUSED(client);
}

void InnerServerHandler::closed(tcp::TcpClient* client) {
  if (client == inner_connection_) {
    fApp->PostEvent(new core::events::ClientDisconnectedEvent(this, authInfo()));
    inner_connection_ = nullptr;
    return;
  }
}

void InnerServerHandler::dataReceived(tcp::TcpClient* client) {
  char buff[MAX_COMMAND_SIZE] = {0};
  ssize_t nread = 0;
  common::Error err = client->read(buff, MAX_COMMAND_SIZE, &nread);
  if ((err && err->isError()) || nread == 0) {
    DEBUG_MSG_ERROR(err);
    client->close();
    delete client;
    return;
  }

  handleInnerDataReceived(dynamic_cast<InnerClient*>(client), buff, nread);
}

void InnerServerHandler::dataReadyToWrite(tcp::TcpClient* client) {}

void InnerServerHandler::postLooped(tcp::ITcpLoop* server) {
  if (inner_connection_) {
    InnerClient* connection = inner_connection_;
    connection->close();
    delete connection;
  }
}

void InnerServerHandler::timerEmited(tcp::ITcpLoop* server, timer_id_t id) {
  if (id == ping_server_id_timer_ && inner_connection_) {
    const cmd_request_t ping_request = make_request(PING_COMMAND_REQ);
    ssize_t nwrite = 0;
    InnerClient* client = inner_connection_;
    common::Error err = client->write(ping_request, &nwrite);
    if (err && err->isError()) {
      DEBUG_MSG_ERROR(err);
      client->close();
      delete client;
    }
  }
}

UserAuthInfo InnerServerHandler::authInfo() const {
  return UserAuthInfo(config_.login, config_.password);
}

void InnerServerHandler::setConfig(const TvConfig& config) {
  config_ = config;
}

void InnerServerHandler::handleInnerRequestCommand(InnerClient* connection,
                                                   cmd_seq_t id,
                                                   int argc,
                                                   char* argv[]) {
  ssize_t nwrite = 0;
  char* command = argv[0];

  if (IS_EQUAL_COMMAND(command, PING_COMMAND)) {
    const cmd_responce_t pong = make_responce(id, PING_COMMAND_RESP_SUCCESS);
    common::Error err = connection->write(pong, &nwrite);
    if (err && err->isError()) {
      DEBUG_MSG_ERROR(err);
    }
  } else if (IS_EQUAL_COMMAND(command, SERVER_WHO_ARE_YOU_COMMAND)) {
    std::string authStr = common::ConvertToString(authInfo());
    cmd_responce_t iAm = make_responce(id, CLIENT_WHO_ARE_YOU_COMMAND_RESP_SUCCSESS_1S, authStr);
    common::Error err = connection->write(iAm, &nwrite);
    if (err && err->isError()) {
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
    json_object_object_add(info_json, "os", json_object_new_string(os.c_str()));
    json_object_object_add(info_json, "cpu", json_object_new_string(brand.c_str()));
    json_object_object_add(info_json, "ram_total", json_object_new_int64(ram_total));
    json_object_object_add(info_json, "ram_free", json_object_new_int64(ram_free));

    const char* info_json_string = json_object_get_string(info_json);
    cmd_responce_t resp =
        make_responce(id, CLIENT_PLEASE_SYSTEM_INFO_COMMAND_RESP_SUCCSESS_1J, info_json_string);
    common::Error err = connection->write(resp, &nwrite);
    if (err && err->isError()) {
      DEBUG_MSG_ERROR(err);
    }
    json_object_put(info_json);
  } else if (IS_EQUAL_COMMAND(command, SERVER_PLEASE_CONFIG_COMMAND)) {
    json_object* config_json = json_object_new_object();
    const char* config_json_string = json_object_get_string(config_json);
    cmd_responce_t resp =
        make_responce(id, CLIENT_PLEASE_CONFIG_COMMAND_RESP_SUCCSESS_1J, config_json_string);
    common::Error err = connection->write(resp, &nwrite);
    if (err && err->isError()) {
      DEBUG_MSG_ERROR(err);
    }

    json_object_put(config_json);
  } else if (IS_EQUAL_COMMAND(command, SERVER_PLEASE_SET_CONFIG_COMMAND)) {
    if (argc > 1) {
      const char* config_json_str = argv[1];
      json_object* config_json = json_tokener_parse(config_json_str);
      if (!config_json) {
        cmd_responce_t resp =
            make_responce(id, CLIENT_PLEASE_SET_CONFIG_COMMAND_RESP_FAIL_1S, CAUSE_INVALID_ARGS);
        common::Error err = connection->write(resp, &nwrite);
        if (err && err->isError()) {
          DEBUG_MSG_ERROR(err);
        }
        return;
      }

      TvConfig new_config;
      new_config.login = config_.login;
      new_config.password = config_.password;

      if (new_config.password != config_.password) {
        cmd_responce_t resp =
            make_responce(id, CLIENT_PLEASE_SET_CONFIG_COMMAND_RESP_FAIL_1S, CAUSE_INVALID_ARGS);
        common::Error err = connection->write(resp, &nwrite);
        if (err && err->isError()) {
          DEBUG_MSG_ERROR(err);
        }
        json_object_put(config_json);
        return;
      }

      cmd_responce_t resp = make_responce(id, CLIENT_PLEASE_SET_CONFIG_COMMAND_RESP_SUCCSESS);
      common::Error err = connection->write(resp, &nwrite);
      if (err && err->isError()) {
        DEBUG_MSG_ERROR(err);
      }
      json_object_put(config_json);

      fApp->PostEvent(new core::events::ClientConfigChangeEvent(this, new_config));
    } else {
      cmd_responce_t resp =
          make_responce(id, CLIENT_PLEASE_SET_CONFIG_COMMAND_RESP_FAIL_1S, CAUSE_INVALID_ARGS);
      common::Error err = connection->write(resp, &nwrite);
      if (err && err->isError()) {
        DEBUG_MSG_ERROR(err);
      }
    }
  } else {
    WARNING_LOG() << "UNKNOWN REQUEST COMMAND: " << command;
  }
}

void InnerServerHandler::handleInnerResponceCommand(InnerClient* connection,
                                                    cmd_seq_t id,
                                                    int argc,
                                                    char* argv[]) {
  ssize_t nwrite = 0;
  char* state_command = argv[0];

  if (IS_EQUAL_COMMAND(state_command, SUCCESS_COMMAND) && argc > 1) {
    char* command = argv[1];
    if (IS_EQUAL_COMMAND(command, PING_COMMAND)) {
      if (argc > 2) {
        const char* pong = argv[2];
        if (!pong) {
          cmd_approve_t resp =
              make_approve_responce(id, PING_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS);
          common::Error err = connection->write(resp, &nwrite);
          if (err && err->isError()) {
            DEBUG_MSG_ERROR(err);
          }
          return;
        }

        const cmd_approve_t resp = make_approve_responce(id, PING_COMMAND_APPROVE_SUCCESS);
        common::Error err = connection->write(resp, &nwrite);
        if (err && err->isError()) {
          DEBUG_MSG_ERROR(err);
          return;
        }
      } else {
        cmd_approve_t resp =
            make_approve_responce(id, PING_COMMAND_APPROVE_FAIL_1S, CAUSE_INVALID_ARGS);
        common::Error err = connection->write(resp, &nwrite);
        if (err && err->isError()) {
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

void InnerServerHandler::handleInnerApproveCommand(InnerClient* connection,
                                                   cmd_seq_t id,
                                                   int argc,
                                                   char* argv[]) {
  UNUSED(connection);
  UNUSED(id);
  char* command = argv[0];

  if (IS_EQUAL_COMMAND(command, SUCCESS_COMMAND)) {
    if (argc > 1) {
      const char* okrespcommand = argv[1];
      if (IS_EQUAL_COMMAND(okrespcommand, PING_COMMAND)) {
      } else if (IS_EQUAL_COMMAND(okrespcommand, SERVER_WHO_ARE_YOU_COMMAND)) {
        fApp->PostEvent(new core::events::ClientConnectedEvent(this, authInfo()));
      }
    }
  } else if (IS_EQUAL_COMMAND(command, FAIL_COMMAND)) {
  } else {
    WARNING_LOG() << "UNKNOWN COMMAND: " << command;
  }
}

}  // namespace inner
}  // namespace fastotv
}  // namespace fasto
