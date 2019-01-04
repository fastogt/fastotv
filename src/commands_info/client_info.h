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

#include <string>

#include <common/serializer/json_serializer.h>

#include "client_server_types.h"  // for bandwidth_t, login_t

namespace fastotv {

class ClientInfo : public common::serializer::JsonSerializer<ClientInfo> {
 public:
  ClientInfo();
  ClientInfo(const login_t& login,
             const std::string& os,
             const std::string& cpu_brand,
             int64_t ram_total,
             int64_t ram_free,
             bandwidth_t bandwidth);

  bool IsValid() const;

  login_t GetLogin() const;
  std::string GetOs() const;
  std::string GetCpuBrand() const;
  int64_t GetRamTotal() const;
  int64_t GetRamFree() const;
  bandwidth_t GetBandwidth() const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* deserialized) const override;

 private:
  login_t login_;
  std::string os_;
  std::string cpu_brand_;
  int64_t ram_total_;
  int64_t ram_free_;
  bandwidth_t bandwidth_;
};

}  // namespace fastotv
