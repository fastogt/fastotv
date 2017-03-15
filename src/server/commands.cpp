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

#include "commands/commands.h"

// requests
// who are you
#define SERVER_WHO_ARE_YOU_COMMAND_REQ GENERATE_FMT(SERVER_WHO_ARE_YOU_COMMAND, "")
#define SERVER_WHO_ARE_YOU_COMMAND_APPROVE_FAIL_1E \
  GENEATATE_FAIL_FMT(SERVER_WHO_ARE_YOU_COMMAND, "'%s'")
#define SERVER_WHO_ARE_YOU_COMMAND_APPROVE_SUCCESS \
  GENEATATE_SUCCESS_FMT(SERVER_WHO_ARE_YOU_COMMAND, "")

// system info
#define SERVER_PLEASE_SYSTEM_INFO_COMMAND_REQ GENERATE_FMT(SERVER_PLEASE_SYSTEM_INFO_COMMAND, "")
#define SERVER_PLEASE_SYSTEM_INFO_COMMAND_APPROVE_FAIL_1E \
  GENERATE_FMT(SERVER_PLEASE_SYSTEM_INFO_COMMAND, "'%s'")
#define SERVER_PLEASE_SYSTEM_INFO_COMMAND_APPROVE_SUCCESS \
  GENERATE_FMT(SERVER_PLEASE_SYSTEM_INFO_COMMAND, "")

// ping
#define SERVER_PING_COMMAND_REQ GENERATE_FMT(SERVER_PING_COMMAND, "")
#define SERVER_PING_COMMAND_APPROVE_FAIL_1E GENEATATE_SUCCESS_FMT(SERVER_PING_COMMAND, "'%s'")
#define SERVER_PING_COMMAND_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(SERVER_PING_COMMAND, "")

// responces
// get_channels
#define SERVER_GET_CHANNELS_COMMAND_RESP_FAIL_1E GENEATATE_FAIL_FMT(CLIENT_GET_CHANNELS, "'%s'")
#define SERVER_GET_CHANNELS_COMMAND_RESP_SUCCSESS_1E \
  GENEATATE_SUCCESS_FMT(CLIENT_GET_CHANNELS, "'%s'")

// ping
#define SERVER_PING_COMMAND_COMMAND_RESP_FAIL_1E GENEATATE_FAIL_FMT(CLIENT_PING_COMMAND, "'%s'")
#define SERVER_PING_COMMAND_COMMAND_RESP_SUCCSESS GENEATATE_SUCCESS_FMT(CLIENT_PING_COMMAND, "pong")

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
  return MakeRequest(id, SERVER_PLEASE_SYSTEM_INFO_COMMAND_REQ);
}
cmd_approve_t SystemInfoApproveResponceSuccsess(cmd_seq_t id) {
  return MakeApproveResponce(id, SERVER_PLEASE_SYSTEM_INFO_COMMAND_APPROVE_SUCCESS);
}
cmd_approve_t SystemInfoApproveResponceFail(cmd_seq_t id, const std::string& error_text) {
  return MakeApproveResponce(id, SERVER_PLEASE_SYSTEM_INFO_COMMAND_APPROVE_FAIL_1E, error_text);
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

cmd_responce_t GetChannelsResponceSuccsess(cmd_seq_t id, const std::string& channels) {
  return MakeResponce(id, SERVER_GET_CHANNELS_COMMAND_RESP_SUCCSESS_1E, channels);
}
cmd_responce_t GetChannelsResponceFail(cmd_seq_t id, const std::string& error_text) {
  return MakeResponce(id, SERVER_GET_CHANNELS_COMMAND_RESP_FAIL_1E, error_text);
}

cmd_responce_t PingResponceSuccsess(cmd_seq_t id) {
  return MakeResponce(id, SERVER_PING_COMMAND_COMMAND_RESP_SUCCSESS);
}
cmd_responce_t PingResponceFail(cmd_seq_t id, const std::string& error_text) {
  return MakeResponce(id, SERVER_PING_COMMAND_COMMAND_RESP_FAIL_1E, error_text);
}
}
}
}
