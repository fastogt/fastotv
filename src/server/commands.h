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
namespace server {

// requests
// who are you
cmd_request_t WhoAreYouRequest(cmd_seq_t id);
cmd_approve_t WhoAreYouApproveResponceSuccsess(cmd_seq_t id);
cmd_approve_t WhoAreYouApproveResponceFail(cmd_seq_t id, const std::string& error_text);  // escaped

// system info
cmd_request_t SystemInfoRequest(cmd_seq_t id);
cmd_approve_t SystemInfoApproveResponceSuccsess(cmd_seq_t id);
cmd_approve_t SystemInfoApproveResponceFail(cmd_seq_t id,
                                            const std::string& error_text);  // escaped

// ping
cmd_request_t PingRequest(cmd_seq_t id);
cmd_approve_t PingApproveResponceSuccsess(cmd_seq_t id);
cmd_approve_t PingApproveResponceFail(cmd_seq_t id, const std::string& error_text);  // escaped

// responces
// get_server_info
cmd_responce_t GetServerInfoResponceSuccsess(cmd_seq_t id,
                                             const std::string& server_info);  // escaped
cmd_responce_t GetServerInfoResponceFail(cmd_seq_t id, const std::string& error_text);

// get_channels
cmd_responce_t GetChannelsResponceSuccsess(cmd_seq_t id,
                                           const std::string& channels_info);  // escaped
cmd_responce_t GetChannelsResponceFail(cmd_seq_t id, const std::string& error_text);

// ping
cmd_responce_t PingResponceSuccsess(cmd_seq_t id, const std::string& ping_info);  // escaped
cmd_responce_t PingResponceFail(cmd_seq_t id, const std::string& error_text);
}  // namespace server
}  // namespace fastotv
}  // namespace fasto
