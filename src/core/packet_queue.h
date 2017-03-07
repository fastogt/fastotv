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

#include "core/types.h"

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

  bool AbortRequest();
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
