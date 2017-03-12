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

#pragma once

#include <inttypes.h>
#include <string>

#include <common/sprintf.h>
#include <common/error.h>

#define END_OF_COMMAND "\r\n"

#define CAUSE_INVALID_ARGS "invalid_args"
#define CAUSE_INVALID_USER "invalid_user"
#define CAUSE_UNREGISTERED_USER "unregistered_user"
#define CAUSE_DOUBLE_CONNECTION_HOST "double_connection_host"
#define CAUSE_CONNECT_FAILED "connect_failed"
#define CAUSE_AUTH_FAILED "auth_failed"
#define CAUSE_PARSE_COMMAND_FAILED "parse_command_failed"
#define CAUSE_INVALID_SEQ "invalid_sequence"
#define CAUSE_NOT_CONNECTED "not_connected"
#define CAUSE_NOT_HANDLED "not_handled"

#define FAIL_COMMAND "fail"
#define SUCCESS_COMMAND "ok"

#define MAX_COMMAND_SIZE 1024
#define IS_EQUAL_COMMAND(BUF, CMD) BUF&& memcmp(BUF, CMD, sizeof(CMD) - 1) == 0

#define CID_FMT PRIu8

#define GENERATE_FMT(CMD, CMD_FMT) "%" CID_FMT " %s " CMD " " CMD_FMT END_OF_COMMAND
#define GENEATATE_SUCCESS_FMT(CMD, CMD_FMT) \
  "%" CID_FMT " %s " SUCCESS_COMMAND " " CMD " " CMD_FMT END_OF_COMMAND
#define GENEATATE_FAIL_FMT(CMD, CMD_FMT) \
  "%" CID_FMT " %s " FAIL_COMMAND " " CMD " " CMD_FMT END_OF_COMMAND

#define REQUEST_COMMAND 0
#define RESPONCE_COMMAND 1
#define APPROVE_COMMAND 2

// client commands
#define CLIENT_PING_COMMAND "client_ping"
#define CLIENT_PING_COMMAND_REQ GENERATE_FMT(CLIENT_PING_COMMAND, "")
#define CLIENT_PING_COMMAND_APPROVE_FAIL_1S GENEATATE_SUCCESS_FMT(CLIENT_PING_COMMAND, "%s")
#define CLIENT_PING_COMMAND_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(CLIENT_PING_COMMAND, "")

#define CLIENT_GET_CHANNELS "get_channels"
#define CLIENT_GET_CHANNELS_REQ GENERATE_FMT(CLIENT_GET_CHANNELS, "")
#define CLIENT_GET_CHANNELS_APPROVE_FAIL_1S GENEATATE_SUCCESS_FMT(CLIENT_GET_CHANNELS, "%s")
#define CLIENT_GET_CHANNELS_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(CLIENT_GET_CHANNELS, "")

// status commands, if request failed common error
#define STATE_COMMAND "state_command"
#define STATE_COMMAND_RESP_FAIL_1S GENEATATE_FAIL_FMT(STATE_COMMAND, "%s")
#define STATE_COMMAND_RESP_SUCCESS GENEATATE_SUCCESS_FMT(STATE_COMMAND, "")

// server commands
#define SERVER_PING_COMMAND "server_ping"
#define SERVER_PING_COMMAND_REQ GENERATE_FMT(SERVER_PING_COMMAND, "")
#define SERVER_PING_COMMAND_APPROVE_FAIL_1S GENEATATE_SUCCESS_FMT(SERVER_PING_COMMAND, "%s")
#define SERVER_PING_COMMAND_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(SERVER_PING_COMMAND, "")

#define SERVER_WHO_ARE_YOU_COMMAND "who_are_you"

#define SERVER_PLEASE_SYSTEM_INFO_COMMAND "plz_system_info"

#define SERVER_PLEASE_CONFIG_COMMAND "plz_config"

#define SERVER_PLEASE_SET_CONFIG_COMMAND "plz_set_config"

// request
// [size_t](0) [hex_string]seq [std::string]command args ...

// responce
// [size_t](1) [hex_string]seq [OK|FAIL] [std::string]command args ...

// approve
// [size_t](2) [hex_string]seq [OK|FAIL] [std::string]command args ...

namespace fasto {
namespace fastotv {

typedef std::string cmd_seq_t;
typedef uint8_t cmd_id_t;

std::string CmdIdToString(cmd_id_t id);

common::Error StableCommand(const std::string& command, std::string* stabled_command);
common::Error ParseCommand(const std::string& command,
                           cmd_id_t* cmd_id,
                           cmd_seq_t* seq_id,
                           std::string* cmd_str);

template <cmd_id_t cmd_id>
class InnerCmd {
 public:
  InnerCmd(cmd_seq_t id, const std::string& cmd) : id_(id), cmd_(cmd) {}

  static cmd_id_t type() { return cmd_id; }

  cmd_seq_t id() const { return id_; }

  const std::string& cmd() const { return cmd_; }

  const char* data() const { return cmd_.c_str(); }

  size_t size() const { return cmd_.size(); }

 private:
  const cmd_seq_t id_;
  const std::string cmd_;
};

typedef InnerCmd<REQUEST_COMMAND> cmd_request_t;
typedef InnerCmd<RESPONCE_COMMAND> cmd_responce_t;
typedef InnerCmd<APPROVE_COMMAND> cmd_approve_t;

}  // namespace fastotv
}  // namespace fasto
