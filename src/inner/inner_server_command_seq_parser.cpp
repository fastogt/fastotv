/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of SiteOnYourDevice.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "inner/inner_server_command_seq_parser.h"

#include <string>

#include <common/convert2string.h>
#include <common/logger.h>

#include "inner/inner_client.h"

extern "C" {
#include "sds.h"
}

#define GB (1024 * 1024 * 1024)
#define BUF_SIZE 4096

namespace fasto {
namespace fastotv {
namespace inner {

RequestCallback::RequestCallback(cmd_seq_t request_id, callback_t cb)
    : request_id_(request_id), cb_(cb) {}

cmd_seq_t RequestCallback::request_id() const {
  return request_id_;
}

void RequestCallback::execute(int argc, char* argv[]) {
  if (!cb_) {
    return;
  }

  return cb_(request_id_, argc, argv);
}

InnerServerCommandSeqParser::InnerServerCommandSeqParser() : id_() {}

InnerServerCommandSeqParser::~InnerServerCommandSeqParser() {}

cmd_seq_t InnerServerCommandSeqParser::NextId() {
  size_t NextId = id_++;
  char bytes[sizeof(size_t)];
  betoh_memcpy(&bytes, &NextId, sizeof(bytes));
  std::string hex = common::HexEncode(&bytes, sizeof(bytes), true);
  return hex;
}

namespace {

bool exec_reqest(RequestCallback req, cmd_seq_t request_id, int argc, char* argv[]) {
  if (request_id == req.request_id()) {
    req.execute(argc, argv);
    return true;
  }

  return false;
}

}  // namespace

void InnerServerCommandSeqParser::ProcessRequest(cmd_seq_t request_id, int argc, char* argv[]) {
  subscribed_requests_.erase(
      std::remove_if(subscribed_requests_.begin(), subscribed_requests_.end(),
                     std::bind(&exec_reqest, std::placeholders::_1, request_id, argc, argv)),
      subscribed_requests_.end());
}

void InnerServerCommandSeqParser::SubscribeRequest(const RequestCallback& req) {
  subscribed_requests_.push_back(req);
}

void InnerServerCommandSeqParser::HandleInnerDataReceived(InnerClient* connection,
                                                          char* buff,
                                                          size_t buff_len) {
  const std::string input_command(buff, buff_len);
  cmd_id_t seq;
  cmd_seq_t id;
  std::string cmd_str;

  common::Error err = ParseCommand(input_command, &seq, &id, &cmd_str);
  if (err && err->isError()) {
    WARNING_LOG() << err->description();
    connection->close();
    delete connection;
    return;
  }

  int argc;
  sds* argv = sdssplitargslong(cmd_str.c_str(), &argc);
  ProcessRequest(id, argc, argv);
  if (argv == NULL) {
    WARNING_LOG() << "PROBLEM PARSING INNER COMMAND: " << buff;
    connection->close();
    delete connection;
    return;
  }

  INFO_LOG() << "HANDLE INNER COMMAND client[" << connection->formatedName()
             << "] seq: " << CmdIdToString(seq) << ", id:" << id << ", cmd: " << cmd_str;
  if (seq == REQUEST_COMMAND) {
    HandleInnerRequestCommand(connection, id, argc, argv);
  } else if (seq == RESPONCE_COMMAND) {
    HandleInnerResponceCommand(connection, id, argc, argv);
  } else if (seq == APPROVE_COMMAND) {
    HandleInnerApproveCommand(connection, id, argc, argv);
  } else {
    DNOTREACHED();
    connection->close();
    delete connection;
  }
  sdsfreesplitres(argv, argc);
}

}  // namespace inner
}  // namespace fastotv
}  // namespace fasto
