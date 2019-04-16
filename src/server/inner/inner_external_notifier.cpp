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

#include "server/inner/inner_external_notifier.h"

#include <common/error.h>   // for Error, DEBUG_MSG_...
#include <common/logger.h>  // for COMPACT_LOG_WARNING
#include <common/macros.h>  // for STRINGIZE
#include <common/protocols/json_rpc/json_rpc.h>

#include "inner/inner_server_command_seq_parser.h"  // for RequestCallback

#include "server/inner/inner_tcp_client.h"
#include "server/inner/inner_tcp_handler.h"
#include "server/user_response_info.h"

// publish COMMANDS_IN '{user_id:'', device_id:'', request : {JSONRPC}} => request

namespace fastotv {
namespace server {
namespace inner {

InnerSubHandler::InnerSubHandler(InnerTcpHandlerHost* parent) : parent_(parent) {}

InnerSubHandler::~InnerSubHandler() {}

void InnerSubHandler::HandleMessage(const std::string& channel, const std::string& msg) {
  INFO_LOG() << "InnerSubHandler channel: " << channel << ", msg: " << msg;
  json_object* jmsg = json_tokener_parse(msg.c_str());
  if (!jmsg) {
    return;
  }

  UserRequestInfo ureq;
  common::Error err = ureq.DeSerialize(jmsg);
  if (err) {
    return;
  }

  common::ErrnoError errn = HandleRequest(ureq);
  if (errn) {
    const protocol::request_t req = ureq.GetRequest();
    const protocol::response_t resp =
        protocol::response_t::MakeError(req.id, protocol::MakeInternalErrorFromText(errn->GetDescription()));
    PublishResponse(ureq, &resp);
  }
}

common::ErrnoError InnerSubHandler::HandleRequest(const UserRequestInfo& request) {
  InnerTcpClient* fclient = parent_->FindInnerConnectionByUserIDAndDeviceID(request.GetUserId(), request.GetDeviceId());
  if (!fclient) {
    common::ErrnoError not_found_user_error = common::make_errno_error("User not found.", EINVAL);
    return not_found_user_error;
  }

  const protocol::request_t req = request.GetRequest();
  auto cb = std::bind(&InnerSubHandler::PublishResponse, this, request, std::placeholders::_1);
  return fclient->WriteRequest(req, cb);
}

void InnerSubHandler::PublishResponse(const UserRequestInfo& uinf, const protocol::response_t* resp) {
  std::string msg;
  UserResponseInfo response(uinf.GetUserId(), uinf.GetDeviceId(), uinf.GetRequest(), *resp);
  common::Error err = response.SerializeToString(&msg);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    return;
  }

  err = parent_->PublishToChannelOut(msg);
  if (err) {
    WARNING_LOG() << "Publish message: " << msg << " to channel out failed.";
  }
}

}  // namespace inner
}  // namespace server
}  // namespace fastotv
