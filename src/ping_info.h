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

#include <common/serializer/json_serializer.h>

#include "client_server_types.h"  // for timestamp_t

namespace fastotv {

class ServerPingInfo : public common::serializer::JsonSerializer<ServerPingInfo> {
 public:
  ServerPingInfo();

  timestamp_t GetTimeStamp() const;

 protected:
  virtual common::Error DoDeSerialize(json_object* serialized) override;
  virtual common::Error SerializeFields(json_object* deserialized) const override;

 private:
  timestamp_t timestamp_;  // utc time
};

class ClientPingInfo : public common::serializer::JsonSerializer<ClientPingInfo> {
 public:
  ClientPingInfo();

  timestamp_t GetTimeStamp() const;

 protected:
  virtual common::Error DoDeSerialize(json_object* serialized) override;
  virtual common::Error SerializeFields(json_object* deserialized) const override;

 private:
  timestamp_t timestamp_;  // utc time
};

}  // namespace fastotv
