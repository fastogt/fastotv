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

#include "server/commands.h"

// requests
// ping
#define SERVER_PING_REQ GENERATE_REQUEST_FMT(SERVER_PING)
#define SERVER_PING_APPROVE_FAIL_1E GENEATATE_FAIL_FMT(SERVER_PING, "'%s'")
#define SERVER_PING_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(SERVER_PING, "")

// who_are_you
#define SERVER_WHO_ARE_YOU_REQ GENERATE_REQUEST_FMT(SERVER_WHO_ARE_YOU)
#define SERVER_WHO_ARE_YOU_APPROVE_FAIL_1E GENEATATE_FAIL_FMT(SERVER_WHO_ARE_YOU, "'%s'")
#define SERVER_WHO_ARE_YOU_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(SERVER_WHO_ARE_YOU, "")

// system_info
#define SERVER_GET_CLIENT_INFO_REQ GENERATE_REQUEST_FMT(SERVER_GET_CLIENT_INFO)
#define SERVER_GET_CLIENT_INFO_APPROVE_FAIL_1E GENEATATE_FAIL_FMT(SERVER_GET_CLIENT_INFO, "'%s'")
#define SERVER_GET_CLIENT_INFO_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(SERVER_GET_CLIENT_INFO, "")

// chat_message
#define SERVER_SEND_CHAT_MESSAGE_REQ_1E GENERATE_REQUEST_FMT_ARGS(SERVER_SEND_CHAT_MESSAGE, "'%s'")
#define SERVER_SEND_CHAT_MESSAGE_APPROVE_FAIL_1E GENEATATE_FAIL_FMT(SERVER_SEND_CHAT_MESSAGE, "'%s'")
#define SERVER_SEND_CHAT_MESSAGE_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(SERVER_SEND_CHAT_MESSAGE, "")

// responces
// get_server_info
#define SERVER_GET_SERVER_INFO_RESP_FAIL_1E GENEATATE_FAIL_FMT(CLIENT_GET_SERVER_INFO, "'%s'")
#define SERVER_GET_SERVER_INFO_RESP_SUCCSESS_1E GENEATATE_SUCCESS_FMT(CLIENT_GET_SERVER_INFO, "'%s'")

// get_channels
#define SERVER_GET_CHANNELS_RESP_FAIL_1E GENEATATE_FAIL_FMT(CLIENT_GET_CHANNELS, "'%s'")
#define SERVER_GET_CHANNELS_RESP_SUCCSESS_1E GENEATATE_SUCCESS_FMT(CLIENT_GET_CHANNELS, "'%s'")

// get_runtime_channel_info
#define SERVER_GET_RUNTIME_CHANNEL_INFO_RESP_FAIL_1E GENEATATE_FAIL_FMT(CLIENT_GET_RUNTIME_CHANNEL_INFO, "'%s'")
#define SERVER_GET_RUNTIME_CHANNEL_INFO_RESP_SUCCSESS_1E GENEATATE_SUCCESS_FMT(CLIENT_GET_RUNTIME_CHANNEL_INFO, "'%s'")

// send_chat_message
#define SERVER_SEND_CHAT_MESSAGE_RESP_FAIL_1E GENEATATE_FAIL_FMT(CLIENT_SEND_CHAT_MESSAGE, "'%s'")
#define SERVER_SEND_CHAT_MESSAGE_RESP_SUCCSESS_1E GENEATATE_SUCCESS_FMT(CLIENT_SEND_CHAT_MESSAGE, "'%s'")

// ping
#define SERVER_PING_RESP_FAIL_1E GENEATATE_FAIL_FMT(CLIENT_PING, "'%s'")
#define SERVER_PING_RESP_SUCCSESS_1E GENEATATE_SUCCESS_FMT(CLIENT_PING, "'%s'")

namespace fastotv {
namespace server {

common::protocols::three_way_handshake::cmd_request_t WhoAreYouRequest(
    common::protocols::three_way_handshake::cmd_seq_t id) {
  return common::protocols::three_way_handshake::MakeRequest(id, SERVER_WHO_ARE_YOU_REQ);
}
common::protocols::three_way_handshake::cmd_approve_t WhoAreYouApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, SERVER_WHO_ARE_YOU_APPROVE_SUCCESS);
}
common::protocols::three_way_handshake::cmd_approve_t WhoAreYouApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, SERVER_WHO_ARE_YOU_APPROVE_FAIL_1E,
                                                                     error_text);
}

common::protocols::three_way_handshake::cmd_request_t SystemInfoRequest(
    common::protocols::three_way_handshake::cmd_seq_t id) {
  return common::protocols::three_way_handshake::MakeRequest(id, SERVER_GET_CLIENT_INFO_REQ);
}
common::protocols::three_way_handshake::cmd_approve_t SystemInfoApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, SERVER_GET_CLIENT_INFO_APPROVE_SUCCESS);
}
common::protocols::three_way_handshake::cmd_approve_t SystemInfoApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, SERVER_GET_CLIENT_INFO_APPROVE_FAIL_1E,
                                                                     error_text);
}

common::protocols::three_way_handshake::cmd_request_t ServerSendChatMessageRequest(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& msg) {
  return common::protocols::three_way_handshake::MakeRequest(id, SERVER_SEND_CHAT_MESSAGE_REQ_1E, msg);
}
common::protocols::three_way_handshake::cmd_approve_t ServerSendChatMessageApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, SERVER_SEND_CHAT_MESSAGE_APPROVE_SUCCESS);
}
common::protocols::three_way_handshake::cmd_approve_t ServerSendChatMessageApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, SERVER_SEND_CHAT_MESSAGE_APPROVE_FAIL_1E,
                                                                     error_text);
}

common::protocols::three_way_handshake::cmd_request_t PingRequest(
    common::protocols::three_way_handshake::cmd_seq_t id) {
  return common::protocols::three_way_handshake::MakeRequest(id, SERVER_PING_REQ);
}
common::protocols::three_way_handshake::cmd_approve_t PingApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, SERVER_PING_APPROVE_SUCCESS);
}
common::protocols::three_way_handshake::cmd_approve_t PingApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, SERVER_PING_APPROVE_FAIL_1E, error_text);
}

common::protocols::three_way_handshake::cmd_responce_t GetServerInfoResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& server_info) {
  return common::protocols::three_way_handshake::MakeResponce(id, SERVER_GET_SERVER_INFO_RESP_SUCCSESS_1E, server_info);
}

common::protocols::three_way_handshake::cmd_responce_t GetServerInfoResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text) {
  return common::protocols::three_way_handshake::MakeResponce(id, SERVER_GET_SERVER_INFO_RESP_FAIL_1E, error_text);
}

common::protocols::three_way_handshake::cmd_responce_t GetChannelsResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& channels_info) {
  return common::protocols::three_way_handshake::MakeResponce(id, SERVER_GET_CHANNELS_RESP_SUCCSESS_1E, channels_info);
}
common::protocols::three_way_handshake::cmd_responce_t GetChannelsResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text) {
  return common::protocols::three_way_handshake::MakeResponce(id, SERVER_GET_CHANNELS_RESP_FAIL_1E, error_text);
}

common::protocols::three_way_handshake::cmd_responce_t GetRuntimeChannelInfoResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& rchannel_info) {
  return common::protocols::three_way_handshake::MakeResponce(id, SERVER_GET_RUNTIME_CHANNEL_INFO_RESP_SUCCSESS_1E,
                                                              rchannel_info);
}
common::protocols::three_way_handshake::cmd_responce_t GetRuntimeChannelInfoResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text) {
  return common::protocols::three_way_handshake::MakeResponce(id, SERVER_GET_RUNTIME_CHANNEL_INFO_RESP_FAIL_1E,
                                                              error_text);
}

common::protocols::three_way_handshake::cmd_responce_t SendChatMessageResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& message) {
  return common::protocols::three_way_handshake::MakeResponce(id, SERVER_SEND_CHAT_MESSAGE_RESP_SUCCSESS_1E, message);
}
common::protocols::three_way_handshake::cmd_responce_t SendChatMessageResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text) {
  return common::protocols::three_way_handshake::MakeResponce(id, SERVER_SEND_CHAT_MESSAGE_RESP_FAIL_1E, error_text);
}

common::protocols::three_way_handshake::cmd_responce_t PingResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& ping_info) {
  return common::protocols::three_way_handshake::MakeResponce(id, SERVER_PING_RESP_SUCCSESS_1E, ping_info);
}
common::protocols::three_way_handshake::cmd_responce_t PingResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text) {
  return common::protocols::three_way_handshake::MakeResponce(id, SERVER_PING_RESP_FAIL_1E, error_text);
}

}  // namespace server
}  // namespace fastotv
