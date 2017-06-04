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

#include "server/inner/inner_external_notifier.h"

extern "C" {
#include "sds.h"
}

#include "server/inner/inner_tcp_handler.h"

#include "server/inner/inner_tcp_client.h"

// publish COMMANDS_IN 'user_id 0 1 ping' 0 => request
// publish COMMANDS_OUT '1 [OK|FAIL] ping args...'
// id cmd cause

namespace fasto {
namespace fastotv {
namespace server {
namespace inner {

InnerSubHandler::InnerSubHandler(InnerTcpHandlerHost* parent) : parent_(parent) {}

InnerSubHandler::~InnerSubHandler() {}

void InnerSubHandler::ProcessSubscribed(cmd_seq_t request_id, int argc, char* argv[]) {  // incoming responce
  const char* state_command = argc > 0 ? argv[0] : FAIL_COMMAND;                         // [OK|FAIL]
  const char* command = argc > 1 ? argv[1] : "null";                                     // command
  const std::string json = argc > 2 ? Decode(argv[2]) : "{}";                            // encoded args

  ResponceInfo resp(request_id, state_command, command, json);
  PublishResponce(resp);
}

void InnerSubHandler::HandleMessage(const std::string& channel, const std::string& msg) {
  // [std::string]login [cmd_id_t]seq [std::string]command args ...
  // [cmd_id_t]seq OK/FAIL [std::string]command args ..
  INFO_LOG() << "InnerSubHandler channel: " << channel << ", msg: " << msg;
  size_t space_pos = msg.find_first_of(' ');
  if (space_pos == std::string::npos) {
    const std::string resp = common::MemSPrintf("UNKNOWN COMMAND: %s", msg);
    WARNING_LOG() << resp;
    return;
  }

  const user_id_t uid = msg.substr(0, space_pos);
  const std::string cmd = msg.substr(space_pos + 1);
  const std::string input_command = common::MemSPrintf(STRINGIZE(REQUEST_COMMAND) " %s" END_OF_COMMAND, cmd);
  cmd_id_t seq;
  cmd_seq_t id;
  std::string cmd_str;
  common::Error err = ParseCommand(input_command, &seq, &id, &cmd_str);
  if (err && err->IsError()) {
    std::string resp = err->Description();
    WARNING_LOG() << resp;
    return;
  }

  InnerTcpClient* fclient = parent_->FindInnerConnectionByID(uid);
  if (!fclient) {
    int argc;
    sds* argv = sdssplitargslong(cmd_str.c_str(), &argc);
    char* command = argv[0];

    ResponceInfo resp(id, FAIL_COMMAND, command, "{\"cause\": \"not connected\"}");
    std::string resp_str;
    common::Error err = resp.SerializeToString(&resp_str);
    if (err && err->IsError()) {
      PublishResponce(resp);
      sdsfreesplitres(argv, argc);
      return;
    }

    WARNING_LOG() << resp_str;
    PublishResponce(resp);
    sdsfreesplitres(argv, argc);
    return;
  }

  cmd_request_t req(id, input_command);
  err = fclient->Write(req);
  if (err && err->IsError()) {
    int argc;
    sds* argv = sdssplitargslong(cmd_str.c_str(), &argc);
    char* command = argv[0];

    ResponceInfo resp(id, FAIL_COMMAND, command, "{\"cause\": \"not handled\"}");
    std::string resp_str;
    common::Error err = resp.SerializeToString(&resp_str);
    if (err && err->IsError()) {
      PublishResponce(resp);
      sdsfreesplitres(argv, argc);
      return;
    }

    WARNING_LOG() << resp_str;
    PublishResponce(resp);
    sdsfreesplitres(argv, argc);
    return;
  }

  auto cb = std::bind(&InnerSubHandler::ProcessSubscribed, this, std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3);
  fasto::fastotv::inner::RequestCallback rc(id, cb);
  parent_->SubscribeRequest(rc);
}

void InnerSubHandler::PublishResponce(const ResponceInfo& resp) {
  std::string resp_str;
  common::Error err = resp.SerializeToString(&resp_str);
  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
    return;
  }

  bool res = parent_->PublishToChannelOut(resp_str);
  if (!res) {
    WARNING_LOG() << "Publish message: " << resp_str << " to channel out failed.";
  }
}

}  // namespace inner
}  // namespace server
}  // namespace fastotv
}  // namespace fasto
