/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

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

#include <string>  // for string

#include "client_server_types.h"

#include "protocol/protocol.h"

#include "commands/commands.h"

namespace fastotv {
namespace server {

// requests
protocol::request_t PingRequest(protocol::sequance_id_t id, protocol::serializet_params_t params);
protocol::request_t ServerSendChatMessageRequest(protocol::sequance_id_t id, protocol::serializet_params_t params);

// responces
protocol::response_t PingResponseSuccess(protocol::sequance_id_t id);

protocol::response_t GetServerInfoResponceSuccsess(protocol::sequance_id_t id, protocol::serializet_params_t params);
protocol::response_t GetServerInfoResponceFail(protocol::sequance_id_t id, const std::string& error_text);

protocol::response_t GetChannelsResponceSuccsess(protocol::sequance_id_t id, protocol::serializet_params_t params);
protocol::response_t GetChannelsResponceFail(protocol::sequance_id_t id, const std::string& error_text);

protocol::response_t GetRuntimeChannelInfoResponceSuccsess(protocol::sequance_id_t id,
                                                           protocol::serializet_params_t params);

protocol::response_t SendChatMessageResponceSuccsess(protocol::sequance_id_t id, protocol::serializet_params_t params);

}  // namespace server
}  // namespace fastotv
