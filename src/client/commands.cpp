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

#include "client/commands.h"

// requests
// ping
#define CLIENT_PING_REQ GENERATE_REQUEST_FMT(CLIENT_PING)
#define CLIENT_PING_APPROVE_FAIL_1E GENEATATE_FAIL_FMT(CLIENT_PING, "'%s'")
#define CLIENT_PING_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(CLIENT_PING, "")

// get_server_info
#define CLIENT_GET_SERVER_INFO_REQ GENERATE_REQUEST_FMT(CLIENT_GET_SERVER_INFO)
#define CLIENT_GET_SERVER_INFO_APPROVE_FAIL_1E GENEATATE_FAIL_FMT(CLIENT_GET_SERVER_INFO, "'%s'")
#define CLIENT_GET_SERVER_INFO_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(CLIENT_GET_SERVER_INFO, "")

// get_channels
#define CLIENT_GET_CHANNELS_REQ GENERATE_REQUEST_FMT(CLIENT_GET_CHANNELS)
#define CLIENT_GET_CHANNELS_APPROVE_FAIL_1E GENEATATE_FAIL_FMT(CLIENT_GET_CHANNELS, "'%s'")
#define CLIENT_GET_CHANNELS_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(CLIENT_GET_CHANNELS, "")

// get_runtime_channel_info
#define CLIENT_GET_RUNTIME_CHANNEL_INFO_REQ_1E GENERATE_REQUEST_FMT_ARGS(CLIENT_GET_RUNTIME_CHANNEL_INFO, "'%s'")
#define CLIENT_GET_RUNTIME_CHANNEL_INFO_APPROVE_FAIL_1E GENEATATE_FAIL_FMT(CLIENT_GET_RUNTIME_CHANNEL_INFO, "'%s'")
#define CLIENT_GET_RUNTIME_CHANNEL_INFO_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(CLIENT_GET_RUNTIME_CHANNEL_INFO, "")

// send_chat_message
#define CLIENT_SEND_CHAT_MESSAGE_REQ_1E GENERATE_REQUEST_FMT_ARGS(CLIENT_SEND_CHAT_MESSAGE, "'%s'")
#define CLIENT_SEND_CHAT_MESSAGE_APPROVE_FAIL_1E GENEATATE_FAIL_FMT(CLIENT_SEND_CHAT_MESSAGE, "'%s'")
#define CLIENT_SEND_CHAT_MESSAGE_APPROVE_SUCCESS GENEATATE_SUCCESS_FMT(CLIENT_SEND_CHAT_MESSAGE, "")

// responces
// who are you
#define CLIENT_WHO_ARE_YOU_RESP_FAIL_1E GENEATATE_FAIL_FMT(SERVER_WHO_ARE_YOU, "'%s'")
#define CLIENT_WHO_ARE_YOU_RESP_SUCCSESS_1E GENEATATE_SUCCESS_FMT(SERVER_WHO_ARE_YOU, "'%s'")

// system info
#define CLIENT_PLEASE_SYSTEM_INFO_RESP_FAIL_1E GENEATATE_FAIL_FMT(SERVER_GET_CLIENT_INFO, "'%s'")
#define CLIENT_PLEASE_SYSTEM_INFO_RESP_SUCCSESS_1E GENEATATE_SUCCESS_FMT(SERVER_GET_CLIENT_INFO, "'%s'")

// ping
#define CLIENT_PING_RESP_FAIL_1E GENEATATE_FAIL_FMT(SERVER_PING, "'%s'")
#define CLIENT_PING_RESP_SUCCSESS_1E GENEATATE_SUCCESS_FMT(SERVER_PING, "'%s'")

// server_send_chat_message
#define CLIENT_SEND_CHAT_MESSAGE_RESP_FAIL_1E GENEATATE_FAIL_FMT(SERVER_SEND_CHAT_MESSAGE, "'%s'")
#define CLIENT_SEND_CHAT_MESSAGE_RESP_SUCCSESS_1E GENEATATE_SUCCESS_FMT(SERVER_SEND_CHAT_MESSAGE, "'%s'")

namespace fastotv {
namespace client {

common::protocols::three_way_handshake::cmd_request_t PingRequest(
    common::protocols::three_way_handshake::cmd_seq_t id) {
  return common::protocols::three_way_handshake::MakeRequest(id, CLIENT_PING_REQ);
}

common::protocols::three_way_handshake::cmd_approve_t PingApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, CLIENT_PING_APPROVE_SUCCESS);
}

common::protocols::three_way_handshake::cmd_approve_t PingApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, CLIENT_PING_APPROVE_FAIL_1E, error_text);
}

common::protocols::three_way_handshake::cmd_request_t GetServerInfoRequest(
    common::protocols::three_way_handshake::cmd_seq_t id) {
  return common::protocols::three_way_handshake::MakeRequest(id, CLIENT_GET_SERVER_INFO_REQ);
}

common::protocols::three_way_handshake::cmd_approve_t GetServerInfoApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, CLIENT_GET_SERVER_INFO_APPROVE_SUCCESS);
}

common::protocols::three_way_handshake::cmd_approve_t GetServerInfoApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, CLIENT_GET_SERVER_INFO_APPROVE_FAIL_1E,
                                                                     error_text);
}

common::protocols::three_way_handshake::cmd_request_t GetChannelsRequest(
    common::protocols::three_way_handshake::cmd_seq_t id) {
  return common::protocols::three_way_handshake::MakeRequest(id, CLIENT_GET_CHANNELS_REQ);
}

common::protocols::three_way_handshake::cmd_approve_t GetChannelsApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, CLIENT_GET_CHANNELS_APPROVE_SUCCESS);
}

common::protocols::three_way_handshake::cmd_approve_t GetChannelsApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, CLIENT_GET_CHANNELS_APPROVE_FAIL_1E,
                                                                     error_text);
}

common::protocols::three_way_handshake::cmd_request_t GetRuntimeChannelInfoRequest(
    common::protocols::three_way_handshake::cmd_seq_t id,
    stream_id sid) {
  return common::protocols::three_way_handshake::MakeRequest(id, CLIENT_GET_RUNTIME_CHANNEL_INFO_REQ_1E, sid);
}

common::protocols::three_way_handshake::cmd_approve_t GetRuntimeChannelInfoApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id,
                                                                     CLIENT_GET_RUNTIME_CHANNEL_INFO_APPROVE_SUCCESS);
}

common::protocols::three_way_handshake::cmd_approve_t GetRuntimeChannelInfoApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text) {
  return common::protocols::three_way_handshake::MakeApproveResponce(
      id, CLIENT_GET_RUNTIME_CHANNEL_INFO_APPROVE_FAIL_1E, error_text);
}

common::protocols::three_way_handshake::cmd_request_t SendChatMessageRequest(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& msg) {
  return common::protocols::three_way_handshake::MakeRequest(id, CLIENT_SEND_CHAT_MESSAGE_REQ_1E, msg);
}

common::protocols::three_way_handshake::cmd_approve_t SendChatMessageApproveResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, CLIENT_GET_SERVER_INFO_APPROVE_SUCCESS);
}

common::protocols::three_way_handshake::cmd_approve_t SendChatMessageApproveResponceFail(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const std::string& error_text) {
  return common::protocols::three_way_handshake::MakeApproveResponce(id, CLIENT_GET_SERVER_INFO_APPROVE_FAIL_1E,
                                                                     error_text);
}

common::protocols::three_way_handshake::cmd_responce_t WhoAreYouResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& auth_serialized) {
  return common::protocols::three_way_handshake::MakeResponce(id, CLIENT_WHO_ARE_YOU_RESP_SUCCSESS_1E, auth_serialized);
}

common::protocols::three_way_handshake::cmd_responce_t SystemInfoResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& system_info) {
  return common::protocols::three_way_handshake::MakeResponce(id, CLIENT_PLEASE_SYSTEM_INFO_RESP_SUCCSESS_1E,
                                                              system_info);
}

common::protocols::three_way_handshake::cmd_responce_t PingResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& ping_info_serialized) {
  return common::protocols::three_way_handshake::MakeResponce(id, CLIENT_PING_RESP_SUCCSESS_1E, ping_info_serialized);
}

common::protocols::three_way_handshake::cmd_responce_t SendChatMessageResponceSuccsess(
    common::protocols::three_way_handshake::cmd_seq_t id,
    const serializet_t& chat_message_serialized) {
  return common::protocols::three_way_handshake::MakeResponce(id, CLIENT_SEND_CHAT_MESSAGE_RESP_SUCCSESS_1E,
                                                              chat_message_serialized);
}

}  // namespace client
}  // namespace fastotv
