#pragma once

extern "C" {
#include <SDL2/SDL_thread.h>
}

#include "core/frame_queue.h"

class Decoder {
 public:
  Decoder(AVCodecContext* avctx,
          PacketQueue* queue,
          SDL_cond* empty_queue_cond,
          int decoder_reorder_pts);
  ~Decoder();

  int start(int (*fn)(void*), void* arg);
  void abort(FrameQueue* fq);
  int decodeFrame(AVFrame* frame, AVSubtitle* sub);
  int pktSerial() const;

  int finished;
  int64_t start_pts;
  AVRational start_pts_tb;

 protected:
  AVCodecContext* avctx_;

 private:
  AVPacket pkt_;
  PacketQueue* const queue_;

  int packet_pending_;
  SDL_cond* const empty_queue_cond_;
  int pkt_serial_;

  int64_t next_pts_;
  AVRational next_pts_tb_;
  SDL_Thread* decoder_tid_;
  int decoder_reorder_pts_;
};

class VideoDecoder : public Decoder {
 public:
  VideoDecoder(AVCodecContext* avctx,
               PacketQueue* queue,
               SDL_cond* empty_queue_cond,
               int decoder_reorder_pts);

  int width() const;
  int height() const;
  int64_t ptsCorrectionNumFaultyDts() const;
  int64_t ptsCorrectionNumFaultyPts() const;
};

typedef VideoDecoder SubDecoder;
