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

namespace fastotv {
namespace server {

protocol::request_t PingRequest(protocol::sequance_id_t id, protocol::serializet_params_t params) {
  protocol::request_t req;
  req.id = id;
  req.method = SERVER_PING;
  req.params = params;
  return req;
}

protocol::request_t ServerSendChatMessageRequest(protocol::sequance_id_t id, protocol::serializet_params_t params) {
  protocol::request_t req;
  req.id = id;
  req.method = SERVER_SEND_CHAT_MESSAGE;
  req.params = params;
  return req;
}

protocol::response_t PingResponseSuccess(protocol::sequance_id_t id) {
  return protocol::response_t::MakeMessage(id, protocol::MakeSuccessMessage());
}

protocol::response_t GetServerInfoResponceSuccsess(protocol::sequance_id_t id, protocol::serializet_params_t params) {
  return protocol::response_t::MakeMessage(id, protocol::MakeSuccessMessage(*params));
}

protocol::response_t GetServerInfoResponceFail(protocol::sequance_id_t id, const std::string& error_text) {
  return protocol::response_t::MakeError(id, protocol::MakeInternalErrorFromText(error_text));
}

protocol::response_t GetChannelsResponceSuccsess(protocol::sequance_id_t id, protocol::serializet_params_t params) {
  return protocol::response_t::MakeMessage(id, protocol::MakeSuccessMessage(*params));
}

protocol::response_t GetChannelsResponceFail(protocol::sequance_id_t id, const std::string& error_text) {
  return protocol::response_t::MakeError(id, protocol::MakeInternalErrorFromText(error_text));
}

protocol::response_t GetRuntimeChannelInfoResponceSuccsess(protocol::sequance_id_t id,
                                                           protocol::serializet_params_t params) {
  return protocol::response_t::MakeMessage(id, protocol::MakeSuccessMessage(*params));
}

protocol::response_t SendChatMessageResponceSuccsess(protocol::sequance_id_t id, protocol::serializet_params_t params) {
  return protocol::response_t::MakeMessage(id, protocol::MakeSuccessMessage(*params));
}

}  // namespace server
}  // namespace fastotv
