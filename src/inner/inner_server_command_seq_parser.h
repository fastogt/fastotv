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

  void SubscribeRequest(const RequestCallback& req);

 protected:
  void HandleInnerDataReceived(InnerClient* connection, char* buff, size_t buff_len);

  cmd_seq_t NextId();  // for requests

 private:
  void ProcessRequest(cmd_seq_t request_id, int argc, char* argv[]);

  virtual void HandleInnerRequestCommand(
      InnerClient* connection,
      cmd_seq_t id,
      int argc,
      char* argv[]) = 0;  // called when argv not NULL and argc > 0 , only responce
  virtual void HandleInnerResponceCommand(
      InnerClient* connection,
      cmd_seq_t id,
      int argc,
      char* argv[]) = 0;  // called when argv not NULL and argc > 0, only approve responce
  virtual void HandleInnerApproveCommand(
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
