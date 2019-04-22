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

#include "client/commands.h"

namespace fastotv {
namespace client {

protocol::request_t ActiveRequest(protocol::sequance_id_t id, protocol::serializet_params_t params) {
  protocol::request_t req;
  req.id = id;
  req.method = CLIENT_ACTIVATE;
  req.params = params;
  return req;
}

protocol::request_t PingRequest(protocol::sequance_id_t id, protocol::serializet_params_t params) {
  protocol::request_t req;
  req.id = id;
  req.method = CLIENT_PING;
  req.params = params;
  return req;
}

protocol::request_t GetServerInfoRequest(protocol::sequance_id_t id) {
  protocol::request_t req;
  req.id = id;
  req.method = CLIENT_GET_SERVER_INFO;
  return req;
}

protocol::request_t GetChannelsRequest(protocol::sequance_id_t id) {
  protocol::request_t req;
  req.id = id;
  req.method = CLIENT_GET_CHANNELS;
  return req;
}

protocol::request_t SendChatMessageRequest(protocol::sequance_id_t id, protocol::serializet_params_t params) {
  protocol::request_t req;
  req.id = id;
  req.method = CLIENT_SEND_CHAT_MESSAGE;
  req.params = params;
  return req;
}

protocol::request_t GetRuntimeChannelInfoRequest(protocol::sequance_id_t id, protocol::serializet_params_t params) {
  protocol::request_t req;
  req.id = id;
  req.method = CLIENT_GET_RUNTIME_CHANNEL_INFO;
  req.params = params;
  return req;
}

protocol::response_t PingResponseSuccess(protocol::sequance_id_t id) {
  return protocol::response_t::MakeMessage(id, protocol::MakeSuccessMessage());
}

protocol::response_t SystemInfoResponceSuccsess(protocol::sequance_id_t id, protocol::serializet_params_t params) {
  return protocol::response_t::MakeMessage(id, protocol::MakeSuccessMessage(*params));
}

protocol::response_t ServerSendChatMessageSuccsess(protocol::sequance_id_t id) {
  return protocol::response_t::MakeMessage(id, protocol::MakeSuccessMessage());
}

}  // namespace client
}  // namespace fastotv
