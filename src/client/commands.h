/*  Copyright (C) 2014-2019 FastoGT. All right reserved.

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

#include "protocol/types.h"

#include "commands/commands.h"

namespace fastotv {
namespace client {

// requests
protocol::request_t ActiveRequest(protocol::sequance_id_t id, protocol::serializet_params_t params);
protocol::request_t PingRequest(protocol::sequance_id_t id, protocol::serializet_params_t params);
protocol::request_t GetServerInfoRequest(protocol::sequance_id_t id);
protocol::request_t GetChannelsRequest(protocol::sequance_id_t id);
protocol::request_t GetRuntimeChannelInfoRequest(protocol::sequance_id_t id, protocol::serializet_params_t params);

// responce
protocol::response_t PingResponseSuccess(protocol::sequance_id_t id);
protocol::response_t SystemInfoResponceSuccsess(protocol::sequance_id_t id, protocol::serializet_params_t params);

}  // namespace client
}  // namespace fastotv
