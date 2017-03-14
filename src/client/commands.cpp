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

#include "client/commands.h"

// requests
// ping
#define CLIENT_PING_COMMAND_REQ GENERATE_FMT(CLIENT_PING_COMMAND, "")
#define CLIENT_PING_COMMAND_APPROVE_FAIL_1E GENEATATE_SUCCESS_FMT(CLIENT_PING_COMMAND, "'%s'")
#define CLIENT_PING_COMMAND_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(CLIENT_PING_COMMAND, "")

// get_channels
#define CLIENT_GET_CHANNELS_REQ GENERATE_FMT(CLIENT_GET_CHANNELS, "")
#define CLIENT_GET_CHANNELS_APPROVE_FAIL_1E GENEATATE_SUCCESS_FMT(CLIENT_GET_CHANNELS, "'%s'")
#define CLIENT_GET_CHANNELS_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(CLIENT_GET_CHANNELS, "")

// responces
// who are you
#define CLIENT_WHO_ARE_YOU_COMMAND_RESP_FAIL_1E \
  GENEATATE_FAIL_FMT(SERVER_WHO_ARE_YOU_COMMAND, "'%s'")
#define CLIENT_WHO_ARE_YOU_COMMAND_RESP_SUCCSESS_1E \
  GENEATATE_SUCCESS_FMT(SERVER_WHO_ARE_YOU_COMMAND, "'%s'")

// system info
#define CLIENT_PLEASE_SYSTEM_INFO_COMMAND_RESP_FAIL_1E \
  GENEATATE_FAIL_FMT(SERVER_PLEASE_SYSTEM_INFO_COMMAND, "'%s'")
#define CLIENT_PLEASE_SYSTEM_INFO_COMMAND_RESP_SUCCSESS_1J \
  GENEATATE_SUCCESS_FMT(SERVER_PLEASE_SYSTEM_INFO_COMMAND, "'%s'")

// ping
#define CLIENT_PING_COMMAND_COMMAND_RESP_FAIL_1E GENEATATE_FAIL_FMT(SERVER_PING_COMMAND, "'%s'")
#define CLIENT_PING_COMMAND_COMMAND_RESP_SUCCSESS GENEATATE_SUCCESS_FMT(SERVER_PING_COMMAND, "pong")

namespace fasto {
namespace fastotv {
namespace client {

cmd_request_t PingRequest(cmd_seq_t id) {
  return MakeRequest(id, CLIENT_PING_COMMAND_REQ);
}

cmd_approve_t PingApproveResponceSuccsess(cmd_seq_t id) {
  return MakeApproveResponce(id, CLIENT_PING_COMMAND_APPROVE_SUCCESS);
}

cmd_approve_t PingApproveResponceFail(cmd_seq_t id, const std::string& error_text) {
  return MakeApproveResponce(id, CLIENT_PING_COMMAND_APPROVE_FAIL_1E, error_text);
}

cmd_request_t GetChannelsRequest(cmd_seq_t id) {
  return MakeRequest(id, CLIENT_GET_CHANNELS_REQ);
}

cmd_approve_t GetChannelsApproveResponceSuccsess(cmd_seq_t id) {
  return MakeApproveResponce(id, CLIENT_GET_CHANNELS_APPROVE_SUCCESS);
}

cmd_approve_t GetChannelsApproveResponceFail(cmd_seq_t id, const std::string& error_text) {
  return MakeApproveResponce(id, CLIENT_GET_CHANNELS_APPROVE_FAIL_1E, error_text);
}

cmd_responce_t WhoAreYouResponceSuccsess(cmd_seq_t id, const std::string& auth) {
  return MakeResponce(id, CLIENT_WHO_ARE_YOU_COMMAND_RESP_SUCCSESS_1E, auth);
}

cmd_responce_t SystemInfoResponceSuccsess(cmd_seq_t id, const std::string& system_info) {
  return MakeResponce(id, CLIENT_PLEASE_SYSTEM_INFO_COMMAND_RESP_SUCCSESS_1J, system_info);
}

cmd_responce_t PingResponceSuccsess(cmd_seq_t id) {
  return MakeResponce(id, CLIENT_PING_COMMAND_COMMAND_RESP_SUCCSESS);
}
}
}
}
