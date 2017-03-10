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

#pragma once

#include <functional>
#include <vector>

#include "commands/commands.h"

namespace fasto {
namespace fastotv {
namespace inner {

class InnerClient;

class RequestCallback {
 public:
  typedef std::function<void(cmd_seq_t request_id, int argc, char* argv[])> callback_t;
  RequestCallback(cmd_seq_t request_id, callback_t cb);
  cmd_seq_t request_id() const;
  void execute(int argc, char* argv[]);

 private:
  cmd_seq_t request_id_;
  callback_t cb_;
};

class InnerServerCommandSeqParser {
 public:
  InnerServerCommandSeqParser();
  virtual ~InnerServerCommandSeqParser();

  template <typename... Args>
  cmd_request_t make_request(const char* cmd_fmt, Args... args) {
    char buff[MAX_COMMAND_SIZE] = {0};
    cmd_seq_t id = next_id();
    int res = common::SNPrintf(buff, MAX_COMMAND_SIZE, cmd_fmt, REQUEST_COMMAND, id, args...);
    CHECK_NE(res, -1);
    return cmd_request_t(id, buff);
  }

  void subscribeRequest(const RequestCallback& req);

 protected:
  void handleInnerDataReceived(InnerClient* connection, const char* buff, size_t buff_len);

  template <typename... Args>
  cmd_responce_t make_responce(cmd_seq_t id, const char* cmd_fmt, Args... args) {
    char buff[MAX_COMMAND_SIZE] = {0};
    int res = common::SNPrintf(buff, MAX_COMMAND_SIZE, cmd_fmt, RESPONCE_COMMAND, id, args...);
    CHECK_NE(res, -1);
    return cmd_responce_t(id, buff);
  }

  template <typename... Args>
  cmd_approve_t make_approve_responce(cmd_seq_t id, const char* cmd_fmt, Args... args) {
    char buff[MAX_COMMAND_SIZE] = {0};
    int res = common::SNPrintf(buff, MAX_COMMAND_SIZE, cmd_fmt, APPROVE_COMMAND, id, args...);
    CHECK_NE(res, -1);
    return cmd_approve_t(id, buff);
  }

 private:
  void processRequest(cmd_seq_t request_id, int argc, char* argv[]);

  cmd_seq_t next_id();
  virtual void handleInnerRequestCommand(
      InnerClient* connection,
      cmd_seq_t id,
      int argc,
      char* argv[]) = 0;  // called when argv not NULL and argc > 0 , only responce
  virtual void handleInnerResponceCommand(
      InnerClient* connection,
      cmd_seq_t id,
      int argc,
      char* argv[]) = 0;  // called when argv not NULL and argc > 0, only approve responce
  virtual void handleInnerApproveCommand(
      InnerClient* connection,
      cmd_seq_t id,
      int argc,
      char* argv[]) = 0;  // called when argv not NULL and argc > 0

  common::atomic<uintmax_t> id_;
  std::vector<RequestCallback> subscribed_requests_;
};

}  // namespace inner
}  // namespace fastotv
}  // namespace fasto
