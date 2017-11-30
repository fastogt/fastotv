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

#pragma once

#include <common/net/types.h>  // for HostAndPort

#include "serializer/json_serializer.h"

namespace fastotv {

class ServerInfo : public JsonSerializerEx {
 public:
  ServerInfo();
  ServerInfo(const common::net::HostAndPort& bandwidth_host);

  static common::Error DeSerialize(const serialize_type& serialized, ServerInfo* obj) WARN_UNUSED_RESULT;

  common::net::HostAndPort GetBandwidthHost() const;

 protected:
  virtual common::Error SerializeFields(json_object* obj) const override;

 private:
  common::net::HostAndPort bandwidth_host_;
};

}  // namespace fastotv
