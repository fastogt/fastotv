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

#include "commands/commands.h"

namespace fastotv {
namespace client {

// requests
// ping
common::protocols::three_way_handshake::cmd_request_t PingRequest(common::protocols::three_way_handshake::cmd_seq_t id);
common::protocols::three_way_handshake::cmd_approve_t PingApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id);
common::protocols::three_way_handshake::cmd_approve_t PingApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text);  // escaped

// get_server_info
common::protocols::three_way_handshake::cmd_request_t GetServerInfoRequest(
    common::protocols::three_way_handshake::cmd_seq_t id);
common::protocols::three_way_handshake::cmd_approve_t GetServerInfoApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id);
common::protocols::three_way_handshake::cmd_approve_t GetServerInfoApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text);  // escaped

// get_channels
common::protocols::three_way_handshake::cmd_request_t GetChannelsRequest(
    common::protocols::three_way_handshake::cmd_seq_t id);
common::protocols::three_way_handshake::cmd_approve_t GetChannelsApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id);
common::protocols::three_way_handshake::cmd_approve_t GetChannelsApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text);  // escaped

// get_runtime_channel_info
common::protocols::three_way_handshake::cmd_request_t GetRuntimeChannelInfoRequest(
    common::protocols::three_way_handshake::cmd_seq_t id,
    stream_id sid);
common::protocols::three_way_handshake::cmd_approve_t GetRuntimeChannelInfoApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id);
common::protocols::three_way_handshake::cmd_approve_t GetRuntimeChannelInfoApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text);

// send_chat_message
common::protocols::three_way_handshake::cmd_request_t SendChatMessageRequest(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& msg);
common::protocols::three_way_handshake::cmd_approve_t SendChatMessageApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id);
common::protocols::three_way_handshake::cmd_approve_t SendChatMessageApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text);

// responces
// who are you
common::protocols::three_way_handshake::cmd_responce_t WhoAreYouResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& auth_serialized);
// system info
common::protocols::three_way_handshake::cmd_responce_t SystemInfoResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& system_info);
// ping
common::protocols::three_way_handshake::cmd_responce_t PingResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& ping_info_serialized);
// send_chat_message
common::protocols::three_way_handshake::cmd_responce_t SendChatMessageResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& chat_message_serialized);

}  // namespace client
}  // namespace fastotv
