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

#include <common/libev/tcp/tcp_client.h>

#include "client_server_types.h"
#include "client/types.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace bandwidth {

class TcpBandwidthClient : public common::libev::tcp::TcpClient {
 public:
  typedef common::libev::tcp::TcpClient base_class;
  enum { max_payload_len = 1400 };
  TcpBandwidthClient(common::libev::IoLoop* server,
                     const common::net::socket_info& info,
                     BandwidthHostType hs);
  const char* ClassName() const override;

  common::Error StartSession(uint16_t ms_betwen_send, common::time64_t duration) WARN_UNUSED_RESULT;

  virtual common::Error Read(char* out, size_t size, size_t* nread) override;

  size_t TotalDownloadedBytes() const;
  bandwidth_t DownloadBytesPerSecond() const;
  BandwidthHostType HostType() const;

 private:
  common::time64_t duration_;
  size_t total_downloaded_bytes_;
  common::time64_t start_ts_;
  bandwidth_t downloaded_bytes_per_sec_;
  const BandwidthHostType host_type_;
};

}  // namespace bandwidth
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
