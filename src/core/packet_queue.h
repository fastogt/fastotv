#pragma once

#include "ffmpeg_config.h"

extern "C" {
#include <libavcodec/avcodec.h>

#include <SDL2/SDL_mutex.h>
}

#include <deque>

/* no AV correction is done if too big error */
#define SUBPICTURE_QUEUE_SIZE 16
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE \
  FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

struct MyAVPacket {
  AVPacket pkt;
  int serial;
};

class PacketQueue {
 public:
  /* packet queue handling */
  PacketQueue();
  ~PacketQueue();

  void flush();
  void abort();
  int put(AVPacket* pkt);
  int put_nullpacket(int stream_index);
  /* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
  int get(AVPacket* pkt, int block, int* serial);
  void start();

  static AVPacket* flush_pkt();

  bool abortRequest() const;
  size_t nbPackets() const;
  int size() const;
  int64_t duration() const;

  int serial;

 private:
  int put_private(AVPacket* pkt);

  std::deque<MyAVPacket*> list_;
  int size_;
  int64_t duration_;
  bool abort_request_;
  SDL_mutex* mutex;
  SDL_cond* cond;
};
