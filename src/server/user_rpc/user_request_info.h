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

#pragma once

#include "server/user_rpc/user_rpc_info.h"

#include "protocol/protocol.h"

namespace fastotv {
namespace server {

class UserRequestInfo : public UserRpcInfo {
 public:
  typedef UserRpcInfo base_class;

  UserRequestInfo();
  UserRequestInfo(const user_id_t& uid, const device_id_t& device_id, const protocol::request_t& req);

  protocol::request_t GetRequest() const;

  bool Equals(const UserRequestInfo& state) const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* deserialized) const override;

 private:
  protocol::request_t req_;
};

}  // namespace server
}  // namespace fastotv
