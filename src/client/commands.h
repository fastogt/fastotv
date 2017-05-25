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

#include "commands/commands.h"

namespace fasto {
namespace fastotv {
namespace client {

// requests
// ping
cmd_request_t PingRequest(cmd_seq_t id);
cmd_approve_t PingApproveResponceSuccsess(cmd_seq_t id);
cmd_approve_t PingApproveResponceFail(cmd_seq_t id, const std::string& error_text);  // escaped

// get_server_info
cmd_request_t GetServerInfoRequest(cmd_seq_t id);
cmd_approve_t GetServerInfoApproveResponceSuccsess(cmd_seq_t id);
cmd_approve_t GetServerInfoApproveResponceFail(cmd_seq_t id, const std::string& error_text);  // escaped

// get_channels
cmd_request_t GetChannelsRequest(cmd_seq_t id);
cmd_approve_t GetChannelsApproveResponceSuccsess(cmd_seq_t id);
cmd_approve_t GetChannelsApproveResponceFail(cmd_seq_t id, const std::string& error_text);  // escaped

// responces
// who are you
cmd_responce_t WhoAreYouResponceSuccsess(cmd_seq_t id, const std::string& auth);  // escaped
// system info
cmd_responce_t SystemInfoResponceSuccsess(cmd_seq_t id, const std::string& system_info);  // escaped
// ping
cmd_responce_t PingResponceSuccsess(cmd_seq_t id);
}
}
}
