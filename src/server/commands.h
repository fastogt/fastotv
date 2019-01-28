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
namespace server {

// requests
// ping server
common::protocols::three_way_handshake::cmd_request_t PingRequest(common::protocols::three_way_handshake::cmd_seq_t id);
common::protocols::three_way_handshake::cmd_approve_t PingApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id);
common::protocols::three_way_handshake::cmd_approve_t PingApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text);  // escaped

// who_are_you
common::protocols::three_way_handshake::cmd_request_t WhoAreYouRequest(
    common::protocols::three_way_handshake::cmd_seq_t id);
common::protocols::three_way_handshake::cmd_approve_t WhoAreYouApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id);
common::protocols::three_way_handshake::cmd_approve_t WhoAreYouApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text);  // escaped

// system_info
common::protocols::three_way_handshake::cmd_request_t SystemInfoRequest(
    common::protocols::three_way_handshake::cmd_seq_t id);
common::protocols::three_way_handshake::cmd_approve_t SystemInfoApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id);
common::protocols::three_way_handshake::cmd_approve_t SystemInfoApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text);  // escaped

// send_chat_message server
common::protocols::three_way_handshake::cmd_request_t ServerSendChatMessageRequest(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& msg);
common::protocols::three_way_handshake::cmd_approve_t ServerSendChatMessageApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id);
common::protocols::three_way_handshake::cmd_approve_t ServerSendChatMessageApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text);

// responces
// get_server_info
common::protocols::three_way_handshake::cmd_response_t GetServerInfoResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& server_info);
common::protocols::three_way_handshake::cmd_response_t GetServerInfoResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text);  // escaped

// get_channels
common::protocols::three_way_handshake::cmd_response_t GetChannelsResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& channels_info);
common::protocols::three_way_handshake::cmd_response_t GetChannelsResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text);  // escaped

// get_runtime_channel_info
common::protocols::three_way_handshake::cmd_response_t GetRuntimeChannelInfoResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& rchannel_info);
common::protocols::three_way_handshake::cmd_response_t GetRuntimeChannelInfoResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text);

// send_chat_message client
common::protocols::three_way_handshake::cmd_response_t SendChatMessageResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& message);
common::protocols::three_way_handshake::cmd_response_t SendChatMessageResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text);

// ping client
common::protocols::three_way_handshake::cmd_response_t PingResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& ping_info);
common::protocols::three_way_handshake::cmd_response_t PingResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text);  // escaped

}  // namespace server
}  // namespace fastotv
