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

#include "client/bandwidth/tcp_bandwidth_client.h"

#include <common/time.h>

namespace {
struct new_session_pkt {
  uint8_t type;
  uint16_t duration;
  uint16_t bw;  // Bw in Mbit/s, only allow integers for now
  uint16_t payload_len;
  uint16_t iat;  // Time between packets in ms, only used for TCP
};
}

namespace fasto {
namespace fastotv {
namespace client {
namespace bandwidth {

TcpBandwidthClient::TcpBandwidthClient(common::libev::IoLoop* server,
                                       const common::net::socket_info& info,
                                       BandwidthHostType hs)
    : base_class(server, info),
      duration_(0),
      total_downloaded_bytes_(0),
      start_ts_(0),
      downloaded_bytes_per_sec_(0),
      host_type_(hs) {}

const char* TcpBandwidthClient::ClassName() const {
  return "TcpBandwidthClient";
}

common::Error TcpBandwidthClient::StartSession(uint16_t ms_betwen_send, common::time64_t duration) {
  static const size_t buff_size = sizeof(struct new_session_pkt);
  char bytes[buff_size] = {0};
  struct new_session_pkt* session_pkt = reinterpret_cast<struct new_session_pkt*>(bytes);
  session_pkt->iat = ms_betwen_send;
  size_t writed = 0;
  common::Error err = Write(bytes, buff_size, &writed);
  if (err && err->IsError()) {
    return err;
  }

  duration_ = duration;
  return common::Error();
}

size_t TcpBandwidthClient::TotalDownloadedBytes() const {
  return total_downloaded_bytes_;
}

bandwidth_t TcpBandwidthClient::DownloadBytesPerSecond() const {
  return downloaded_bytes_per_sec_;
}

BandwidthHostType TcpBandwidthClient::HostType() const {
  return host_type_;
}

common::Error TcpBandwidthClient::Read(char* out, size_t size, size_t* nread) {
  common::Error err = base_class::Read(out, size, nread);
  if (err && err->IsError()) {
    return err;
  }

  common::time64_t cur_ts = common::time::current_mstime();
  if (total_downloaded_bytes_ == 0) {
    start_ts_ = cur_ts;
  }
  total_downloaded_bytes_ += *nread;

  const common::time64_t data_interval = cur_ts - start_ts_;
  if (duration_ && data_interval >= duration_) {
    bandwidth_t butes_per_msec = total_downloaded_bytes_ / data_interval;
    bandwidth_t butes_per_sec = butes_per_msec * 1000;
    downloaded_bytes_per_sec_ = butes_per_sec;
    Close();
  }
  return common::Error();
}

}  // namespace bandwidth
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
