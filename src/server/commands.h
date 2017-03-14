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

#include "commands/commands.h"

// publish COMMANDS_IN 'host 0 1 ping' 0 => request
// publish COMMANDS_OUT '1 [OK|FAIL] ping args...'

// for publish only
// id cmd cuase
#define SERVER_COMMANDS_OUT_FAIL_3SEE "%s " FAIL_COMMAND " '%s' '%s'"

#define SERVER_NOTIFY_CLIENT_CONNECTED_1S "%s connected"
#define SERVER_NOTIFY_CLIENT_DISCONNECTED_1S "%s disconnected"

namespace fasto {
namespace fastotv {
namespace server {

// requests
// who are you
cmd_request_t WhoAreYouRequest(cmd_seq_t id);
cmd_approve_t WhoAreYouApproveResponceSuccsess(cmd_seq_t id);
cmd_approve_t WhoAreYouApproveResponceFail(cmd_seq_t id, const std::string& error_text);

// system info
cmd_request_t SystemInfoRequest(cmd_seq_t id);
cmd_approve_t SystemInfoApproveResponceSuccsess(cmd_seq_t id);
cmd_approve_t SystemInfoApproveResponceFail(cmd_seq_t id, const std::string& error_text);

// ping
cmd_request_t PingRequest(cmd_seq_t id);
cmd_approve_t PingApproveResponceSuccsess(cmd_seq_t id);
cmd_approve_t PingApproveResponceFail(cmd_seq_t id, const std::string& error_text);

// responces
// get_channels
cmd_responce_t GetChannelsResponceSuccsess(cmd_seq_t id, const std::string& channels);
cmd_responce_t GetChannelsResponceFail(cmd_seq_t id, const std::string& error_text);

// ping
cmd_responce_t PingResponceSuccsess(cmd_seq_t id);
cmd_responce_t PingResponceFail(cmd_seq_t id, const std::string& error_text);

}
}
}
