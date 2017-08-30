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

#include <stdint.h>  // for int64_t

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

#include "ffmpeg_config.h"

extern "C" {
#include <libavcodec/avcodec.h>  // for AVPacket
}

#include <common/macros.h>  // for DISALLOW_COPY_AND_ASSIGN
#include <common/types.h>

namespace fastotv {
namespace client {
namespace player {
namespace media {

class PacketQueue {  // compressed queue data
 public:
  PacketQueue();
  ~PacketQueue();

  void Flush();
  void Abort();
  int Put(AVPacket* pkt);
  int PutNullpacket(int stream_index);
  /* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
  bool Get(AVPacket* pkt);
  void Start();

  bool IsAborted();
  size_t GetNbPackets();
  int GetSize() const;
  int64_t GetDuration() const;

 private:
  int PushFront(AVPacket* pkt);
  int PushBack(AVPacket* pkt);
  DISALLOW_COPY_AND_ASSIGN(PacketQueue);

  std::deque<AVPacket> queue_;
  std::atomic<int> size_;
  std::atomic<int64_t> duration_;
  bool abort_request_;
  typedef std::unique_lock<std::mutex> lock_t;
  std::condition_variable cond_;
  std::mutex mutex_;
};

}  // namespace media
}  // namespace player
}  // namespace client
}  // namespace fastotv
