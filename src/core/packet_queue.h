#pragma once

#include "ffmpeg_config.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <deque>
#include <atomic>

#include <common/macros.h>
#include <common/types.h>
#include <common/threads/types.h>

namespace core {

struct SAVPacket {
  explicit SAVPacket(const AVPacket& p, int serial);

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
  int Get(AVPacket* pkt, bool block, int* serial);
  void Start();

  static AVPacket* FlushPkt();
  static PacketQueue* MakePacketQueue(common::atomic<int>** ext_serial);

  bool AbortRequest();
  size_t NbPackets();
  int Size();
  int64_t Duration() const;
  int Serial() const;

 private:
  PacketQueue();

  DISALLOW_COPY_AND_ASSIGN(PacketQueue);
  int PutPrivate(SAVPacket* pkt1);

  SAVPacket* MakePacket(AVPacket* pkt, bool* is_flush);

  common::atomic<int> serial_;
  std::deque<SAVPacket*> queue_;
  int size_;
  int64_t duration_;
  bool abort_request_;
  typedef common::unique_lock<common::mutex> lock_t;
  common::condition_variable cond_;
  common::mutex mutex_;
};

}  // namespace core
