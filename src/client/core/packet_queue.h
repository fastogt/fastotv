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

#include <stddef.h>  // for size_t
#include <stdint.h>  // for int64_t
#include <atomic>    // for atomic
#include <deque>     // for deque
#include <mutex>     // for unique_loc

#include <common/macros.h>         // for DISALLOW_COPY_AND_ASSIGN
#include <common/threads/types.h>  // for mutex, condition_variable
#include <common/types.h>

#include "ffmpeg_config.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "client/core/types.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

struct SAVPacket {
  explicit SAVPacket(const AVPacket& p, serial_id_t serial);

  AVPacket pkt;
  const int serial;
};

class PacketQueue {  // compressed queue data
 public:
  ~PacketQueue();

  void Flush();
  void Abort();
  int Put(AVPacket* pkt);
  int PutNullpacket(int stream_index);
  /* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
  int Get(AVPacket* pkt, bool block, serial_id_t* serial);
  void Start();

  static AVPacket* FlushPkt();
  static PacketQueue* MakePacketQueue(common::atomic<serial_id_t>** ext_serial);

  bool IsAborted();
  size_t NbPackets();
  int Size() const;
  int64_t Duration() const;
  serial_id_t Serial() const;

 private:
  PacketQueue();

  DISALLOW_COPY_AND_ASSIGN(PacketQueue);
  int PutPrivate(SAVPacket* pkt1);

  SAVPacket* MakePacket(AVPacket* pkt, bool* is_flush);

  common::atomic<serial_id_t> serial_;
  std::deque<SAVPacket*> queue_;
  int size_;
  int64_t duration_;
  bool abort_request_;
  typedef common::unique_lock<common::mutex> lock_t;
  common::condition_variable cond_;
  common::mutex mutex_;
};

}  // namespace core
}
}
}
