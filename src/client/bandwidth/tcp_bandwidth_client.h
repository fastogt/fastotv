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

#include <common/libev/tcp/tcp_client.h>  // for TcpClient

#include "client/types.h"  // for BandwidthHostType

#include <fastotv/client_server_types.h>  // for bandwidth_t

namespace fastotv {
namespace client {
namespace bandwidth {

class TcpBandwidthClient : public common::libev::tcp::TcpClient {
 public:
  typedef common::libev::tcp::TcpClient base_class;
  enum { max_payload_len = 1400 };
  TcpBandwidthClient(common::libev::IoLoop* server, const common::net::socket_info& info, BandwidthHostType hs);
  const char* ClassName() const override;

  common::ErrnoError StartSession(uint16_t ms_betwen_send, common::time64_t duration) WARN_UNUSED_RESULT;

  common::ErrnoError SingleRead(void* out, size_t size, size_t* nread) override;

  size_t GetTotalDownloadedBytes() const;
  bandwidth_t GetDownloadBytesPerSecond() const;
  BandwidthHostType GetHostType() const;

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
