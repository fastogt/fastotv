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

// publish COMMANDS_IN 'host 0 1 ping' 0 => request
// publish COMMANDS_OUT '1 [OK|FAIL] ping args...'
// id cmd cuase
#define SERVER_COMMANDS_OUT_FAIL_3SEE "%s " FAIL_COMMAND " '%s' '%s'"

namespace fasto {
namespace fastotv {
namespace server {
namespace inner {

InnerSubHandler::InnerSubHandler(InnerTcpHandlerHost* parent) : parent_(parent) {}

InnerSubHandler::~InnerSubHandler() {}

void InnerSubHandler::processSubscribed(cmd_seq_t request_id, int argc, char* argv[]) {
  std::string join = request_id;
  for (int i = 0; i < argc; ++i) {
    join += " ";
    join += argv[i];
  }

  PublishToChannelOut(join);
}

void InnerSubHandler::handleMessage(const std::string& channel, const std::string& msg) {
  // [std::string]site [cmd_id_t]seq [std::string]command args ...
  // [cmd_id_t]seq OK/FAIL [std::string]command args ..
  INFO_LOG() << "InnerSubHandler channel: " << channel << ", msg: " << msg;
  size_t space_pos = msg.find_first_of(' ');
  if (space_pos == std::string::npos) {
    const std::string resp = common::MemSPrintf("UNKNOWN COMMAND: %s", msg);
    WARNING_LOG() << resp;
    PublishToChannelOut(resp);
    return;
  }

  const user_id_t uid = msg.substr(0, space_pos);
  const std::string cmd = msg.substr(space_pos + 1);
  const std::string input_command =
      common::MemSPrintf(STRINGIZE(REQUEST_COMMAND) " %s" END_OF_COMMAND, cmd);
  cmd_id_t seq;
  cmd_seq_t id;
  std::string cmd_str;
  common::Error err = ParseCommand(input_command, &seq, &id, &cmd_str);
  if (err && err->IsError()) {
    std::string resp = err->Description();
    WARNING_LOG() << resp;
    PublishToChannelOut(resp);
    return;
  }

  InnerTcpClient* fclient = parent_->FindInnerConnectionByID(uid);
  if (!fclient) {
    int argc;
    sds* argv = sdssplitargslong(cmd_str.c_str(), &argc);
    char* command = argv[0];

    std::string resp =
        common::MemSPrintf(SERVER_COMMANDS_OUT_FAIL_3SEE, id, command, "Not connected");
    WARNING_LOG() << resp;
    PublishToChannelOut(resp);
    sdsfreesplitres(argv, argc);
    return;
  }

  size_t nwrite = 0;
  cmd_request_t req(id, input_command);
  err = fclient->Write(req, &nwrite);
  if (err && err->IsError()) {
    int argc;
    sds* argv = sdssplitargslong(cmd_str.c_str(), &argc);
    char* command = argv[0];

    std::string resp =
        common::MemSPrintf(SERVER_COMMANDS_OUT_FAIL_3SEE, id, command, "Not handled");
    WARNING_LOG() << resp;
    PublishToChannelOut(resp);
    sdsfreesplitres(argv, argc);
    return;
  }

  auto cb = std::bind(&InnerSubHandler::processSubscribed, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3);
  fasto::fastotv::inner::RequestCallback rc(id, cb);
  parent_->SubscribeRequest(rc);
}

void InnerSubHandler::PublishToChannelOut(const std::string& msg) {
  bool res = parent_->PublishToChannelOut(msg);
  if (!res) {
    WARNING_LOG() << "Publish message: " << msg << " to channel out failed.";
  }
}

}  // namespace inner
}  // namespace server
}  // namespace fastotv
}  // namespace fasto
