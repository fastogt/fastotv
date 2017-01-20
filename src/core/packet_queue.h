#pragma once

#include "ffmpeg_config.h"

extern "C" {
#include <libavcodec/avcodec.h>

#include <SDL2/SDL_mutex.h>
}

#include <deque>

#include <common/macros.h>

struct SAVPacket {
  explicit SAVPacket(const AVPacket& p);

  AVPacket pkt;
  int serial;
};

class PacketQueue {  // compressed queue data
 public:
  ~PacketQueue();

  void flush();
  void abort();
  int put(AVPacket* pkt);
  int put_nullpacket(int stream_index);
  /* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
  int get(AVPacket* pkt, int block, int* serial);
  void start();

  static AVPacket* flush_pkt();
  static PacketQueue* make_packet_queue(int** ext_serial);

  bool abort_request() const;
  size_t nb_packets() const;
  int size() const;
  int64_t duration() const;
  int serial() const;

 private:
  PacketQueue();

  DISALLOW_COPY_AND_ASSIGN(PacketQueue);
  int put_private(AVPacket* pkt);

  int serial_;
  std::deque<SAVPacket*> list_;
  int size_;
  int64_t duration_;
  bool abort_request_;
  SDL_mutex* mutex;
  SDL_cond* cond;
};
