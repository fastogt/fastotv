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

#include "server/commands.h"

// requests
// who are you
#define SERVER_WHO_ARE_YOU_COMMAND_REQ GENERATE_REQUEST_FMT(SERVER_WHO_ARE_YOU_COMMAND)
#define SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1E \
  GENEATATE_FAIL_FMT(SERVER_WHO_ARE_YOU_COMMAND, "'%s'")
#define SERVER_WHO_ARE_YOU_COMMAND_APPROVE_SUCCESS \
  GENEATATE_SUCCESS_FMT(SERVER_WHO_ARE_YOU_COMMAND, "")

// system info
#define SERVER_GET_CLIENT_INFO_COMMAND_REQ GENERATE_REQUEST_FMT(SERVER_GET_CLIENT_INFO_COMMAND)
#define SERVER_GET_CLIENT_INFO_COMMAND_APPROVE_FAIL_1E \
  GENEATATE_FAIL_FMT(SERVER_GET_CLIENT_INFO_COMMAND, "'%s'")
#define SERVER_GET_CLIENT_INFO_COMMAND_APPROVE_SUCCESS \
  GENEATATE_SUCCESS_FMT(SERVER_GET_CLIENT_INFO_COMMAND, "")

// ping
#define SERVER_PING_COMMAND_REQ GENERATE_REQUEST_FMT(SERVER_PING_COMMAND)
#define SERVER_PING_COMMAND_APPROVE_FAIL_1E GENEATATE_FAIL_FMT(SERVER_PING_COMMAND, "'%s'")
#define SERVER_PING_COMMAND_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(SERVER_PING_COMMAND, "")

// responces
// get_server_info
#define SERVER_GET_SERVER_INFO_COMMAND_RESP_FAIL_1E \
  GENEATATE_FAIL_FMT(CLIENT_GET_SERVER_INFO, "'%s'")
#define SERVER_GET_SERVER_INFO_COMMAND_RESP_SUCCSESS_1E \
  GENEATATE_SUCCESS_FMT(CLIENT_GET_SERVER_INFO, "'%s'")

// get_channels
#define SERVER_GET_CHANNELS_COMMAND_RESP_FAIL_1E GENEATATE_FAIL_FMT(CLIENT_GET_CHANNELS, "'%s'")
#define SERVER_GET_CHANNELS_COMMAND_RESP_SUCCSESS_1E \
  GENEATATE_SUCCESS_FMT(CLIENT_GET_CHANNELS, "'%s'")

// ping
#define SERVER_PING_COMMAND_COMMAND_RESP_FAIL_1E GENEATATE_FAIL_FMT(CLIENT_PING_COMMAND, "'%s'")
#define SERVER_PING_COMMAND_COMMAND_RESP_SUCCSESS_1E \
  GENEATATE_SUCCESS_FMT(CLIENT_PING_COMMAND, "'%s'")

namespace fasto {
namespace fastotv {
namespace server {

cmd_request_t WhoAreYouRequest(cmd_seq_t id) {
  return MakeRequest(id, SERVER_WHO_ARE_YOU_COMMAND_REQ);
}
cmd_approve_t WhoAreYouApproveResponceSuccsess(cmd_seq_t id) {
  return MakeApproveResponce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_SUCCESS);
}
cmd_approve_t WhoAreYouApproveResponceFail(cmd_seq_t id, const std::string& error_text) {
  return MakeApproveResponce(id, SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1E, error_text);
}

cmd_request_t SystemInfoRequest(cmd_seq_t id) {
  return MakeRequest(id, SERVER_GET_CLIENT_INFO_COMMAND_REQ);
}
cmd_approve_t SystemInfoApproveResponceSuccsess(cmd_seq_t id) {
  return MakeApproveResponce(id, SERVER_GET_CLIENT_INFO_COMMAND_APPROVE_SUCCESS);
}
cmd_approve_t SystemInfoApproveResponceFail(cmd_seq_t id, const std::string& error_text) {
  return MakeApproveResponce(id, SERVER_GET_CLIENT_INFO_COMMAND_APPROVE_FAIL_1E, error_text);
}

cmd_request_t PingRequest(cmd_seq_t id) {
  return MakeRequest(id, SERVER_PING_COMMAND_REQ);
}
cmd_approve_t PingApproveResponceSuccsess(cmd_seq_t id) {
  return MakeApproveResponce(id, SERVER_PING_COMMAND_APPROVE_SUCCESS);
}
cmd_approve_t PingApproveResponceFail(cmd_seq_t id, const std::string& error_text) {
  return MakeApproveResponce(id, SERVER_PING_COMMAND_APPROVE_FAIL_1E, error_text);
}

cmd_responce_t GetServerInfoResponceSuccsess(cmd_seq_t id, const std::string& server_info) {
  return MakeResponce(id, SERVER_GET_SERVER_INFO_COMMAND_RESP_SUCCSESS_1E, server_info);
}

cmd_responce_t GetServerInfoResponceFail(cmd_seq_t id, const std::string& error_text) {
  return MakeResponce(id, SERVER_GET_SERVER_INFO_COMMAND_RESP_FAIL_1E, error_text);
}

cmd_responce_t GetChannelsResponceSuccsess(cmd_seq_t id, const std::string& channels_info) {
  return MakeResponce(id, SERVER_GET_CHANNELS_COMMAND_RESP_SUCCSESS_1E, channels_info);
}
cmd_responce_t GetChannelsResponceFail(cmd_seq_t id, const std::string& error_text) {
  return MakeResponce(id, SERVER_GET_CHANNELS_COMMAND_RESP_FAIL_1E, error_text);
}

cmd_responce_t PingResponceSuccsess(cmd_seq_t id, const std::string& ping_info) {
  return MakeResponce(id, SERVER_PING_COMMAND_COMMAND_RESP_SUCCSESS_1E, ping_info);
}
cmd_responce_t PingResponceFail(cmd_seq_t id, const std::string& error_text) {
  return MakeResponce(id, SERVER_PING_COMMAND_COMMAND_RESP_FAIL_1E, error_text);
}
}
}
}
