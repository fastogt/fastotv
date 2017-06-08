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

#include "inner/inner_server_command_seq_parser.h"

#include <stddef.h>  // for NULL
#include <string>    // for string

#include <common/error.h>   // for Error
#include <common/logger.h>  // for COMPACT_LOG_WARNING, WARNING_LOG
#include <common/macros.h>  // for betoh_memcpy, DNOTREACHED
#include <common/convert2string.h>

#include "inner/inner_client.h"  // for InnerClient

extern "C" {
#include "sds.h"  // for sdsfreesplitres, sds
}

#define GB (1024 * 1024 * 1024)
#define BUF_SIZE 4096

namespace fasto {
namespace fastotv {
namespace inner {

RequestCallback::RequestCallback(cmd_seq_t request_id, callback_t cb) : request_id_(request_id), cb_(cb) {
}

cmd_seq_t RequestCallback::GetRequestID() const {
  return request_id_;
}

void RequestCallback::Execute(int argc, char* argv[]) {
  if (!cb_) {
    return;
  }

  return cb_(request_id_, argc, argv);
}

InnerServerCommandSeqParser::InnerServerCommandSeqParser() : id_() {
}

InnerServerCommandSeqParser::~InnerServerCommandSeqParser() {
}

cmd_seq_t InnerServerCommandSeqParser::NextRequestID() {
  id_t next_id = id_++;
  char bytes[sizeof(id_t)];
  betoh_memcpy(&bytes, &next_id, sizeof(id_t));
  cmd_seq_t hexed = common::HexEncode(&bytes, sizeof(id_t), true);
  return hexed;
}

namespace {

bool exec_reqest(RequestCallback req, cmd_seq_t request_id, int argc, char* argv[]) {
  if (request_id == req.GetRequestID()) {
    req.Execute(argc, argv);
    return true;
  }

  return false;
}

}  // namespace

void InnerServerCommandSeqParser::ProcessRequest(cmd_seq_t request_id, int argc, char* argv[]) {
  subscribed_requests_.erase(std::remove_if(subscribed_requests_.begin(),
                                            subscribed_requests_.end(),
                                            std::bind(&exec_reqest, std::placeholders::_1, request_id, argc, argv)),
                             subscribed_requests_.end());
}

void InnerServerCommandSeqParser::SubscribeRequest(const RequestCallback& req) {
  subscribed_requests_.push_back(req);
}

void InnerServerCommandSeqParser::HandleInnerDataReceived(InnerClient* connection, const std::string& input_command) {
  cmd_id_t seq;
  cmd_seq_t id;
  std::string cmd_str;

  common::Error err = ParseCommand(input_command, &seq, &id, &cmd_str);
  if (err && err->IsError()) {
    WARNING_LOG() << err->Description();
    connection->Close();
    delete connection;
    return;
  }

  int argc;
  sds* argv = sdssplitargslong(cmd_str.c_str(), &argc);
  if (argv == NULL) {
    const std::string error_str = "PROBLEM PARSING INNER COMMAND: " + input_command;
    WARNING_LOG() << error_str;
    connection->Close();
    delete connection;
    return;
  }

  ProcessRequest(id, argc, argv);
  INFO_LOG() << "HANDLE INNER COMMAND client[" << connection->FormatedName() << "] seq: " << CmdIdToString(seq)
             << ", id:" << id << ", cmd: " << cmd_str;
  if (seq == REQUEST_COMMAND) {
    HandleInnerRequestCommand(connection, id, argc, argv);
  } else if (seq == RESPONCE_COMMAND) {
    HandleInnerResponceCommand(connection, id, argc, argv);
  } else if (seq == APPROVE_COMMAND) {
    HandleInnerApproveCommand(connection, id, argc, argv);
  } else {
    DNOTREACHED();
    connection->Close();
    delete connection;
  }
  sdsfreesplitres(argv, argc);
}

}  // namespace inner
}  // namespace fastotv
}  // namespace fasto
