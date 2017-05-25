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

#pragma once

#include <inttypes.h>
#include <string>

#include <common/sprintf.h>
#include <common/error.h>

#define END_OF_COMMAND "\r\n"

#define FAIL_COMMAND "fail"
#define SUCCESS_COMMAND "ok"

#define IS_EQUAL_COMMAND(BUF, CMD) BUF&& strncmp(BUF, CMD, sizeof(CMD) - 1) == 0

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
#define CLIENT_PING_COMMAND "client_ping"  // ping server
#define CLIENT_GET_SERVER_INFO "get_server_info"
#define CLIENT_GET_CHANNELS "get_channels"

// server commands
#define SERVER_PING_COMMAND "server_ping"  // ping client
#define SERVER_WHO_ARE_YOU_COMMAND "who_are_you"
#define SERVER_PLEASE_SYSTEM_INFO_COMMAND "plz_system_info"

// request
// [uint8_t](0) [hex_string]seq [std::string]command args ...

// responce
// [uint8_t](1) [hex_string]seq [OK|FAIL] [std::string]command args ...

// approve
// [uint8_t](2) [hex_string]seq [OK|FAIL] [std::string]command args ...

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

 private:
  const cmd_seq_t id_;
  const std::string cmd_;
};

typedef InnerCmd<REQUEST_COMMAND> cmd_request_t;
typedef InnerCmd<RESPONCE_COMMAND> cmd_responce_t;
typedef InnerCmd<APPROVE_COMMAND> cmd_approve_t;

template <typename... Args>
cmd_request_t MakeRequest(cmd_seq_t id, const char* cmd_fmt, Args... args) {
  std::string buff = common::MemSPrintf(cmd_fmt, REQUEST_COMMAND, id, args...);
  return cmd_request_t(id, buff);
}

template <typename... Args>
cmd_approve_t MakeApproveResponce(cmd_seq_t id, const char* cmd_fmt, Args... args) {
  std::string buff = common::MemSPrintf(cmd_fmt, APPROVE_COMMAND, id, args...);
  return cmd_approve_t(id, buff);
}

template <typename... Args>
cmd_responce_t MakeResponce(cmd_seq_t id, const char* cmd_fmt, Args... args) {
  std::string buff = common::MemSPrintf(cmd_fmt, RESPONCE_COMMAND, id, args...);
  return cmd_responce_t(id, buff);
}

}  // namespace fastotv
}  // namespace fasto
