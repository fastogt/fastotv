/*  Copyright (C) 2014-2019 FastoGT. All right reserved.

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

#pragma once

#include <atomic>
#include <functional>

#include "protocol/types.h"

namespace fastotv {
namespace inner {
class InnerClient;

class InnerServerCommandSeqParser {
 public:
  InnerServerCommandSeqParser();
  virtual ~InnerServerCommandSeqParser();

 protected:
  common::ErrnoError HandleInnerDataReceived(InnerClient* client, const std::string& input_command);
  virtual common::ErrnoError HandleRequestCommand(InnerClient* client, protocol::request_t* req) = 0;
  virtual common::ErrnoError HandleResponceCommand(InnerClient* client, protocol::response_t* resp) = 0;

  protocol::sequance_id_t NextRequestID();  // for requests

 private:
  std::atomic<protocol::seq_id_t> id_;
};

}  // namespace inner
}  // namespace fastotv
