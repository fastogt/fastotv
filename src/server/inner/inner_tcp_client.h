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

#include "commands_info/auth_info.h"  // for AuthInfo

#include "inner/inner_client.h"  // for InnerClient

#include "server/user_info.h"  // for user_id_t

#include "commands_info/chat_message.h"

namespace common {
namespace libev {
namespace tcp {
class TcpServer;
}
}  // namespace libev
}  // namespace common
namespace common {
namespace net {
class socket_info;
}
}  // namespace common

namespace fastotv {
namespace server {
namespace inner {

class InnerTcpClient : public fastotv::inner::InnerClient {
 public:
  static const AuthInfo anonim_user;

  InnerTcpClient(common::libev::tcp::TcpServer* server, const common::net::socket_info& info);
  ~InnerTcpClient();

  const char* ClassName() const override;

  void SetServerHostInfo(const AuthInfo& info);
  AuthInfo GetServerHostInfo() const;

  void SetUid(user_id_t id);
  user_id_t GetUid() const;

  void SetCurrentStreamId(stream_id sid);
  user_id_t GetCurrentStreamId() const;

  bool IsAnonimUser() const;

 private:
  AuthInfo hinfo_;
  user_id_t uid_;
  stream_id current_stream_id_;
};

}  // namespace inner
}  // namespace server
}  // namespace fastotv
