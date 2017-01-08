#pragma once

extern "C" {
#include <SDL2/SDL_thread.h>
}

#include "core/frame_queue.h"

static int decoder_reorder_pts = -1;

typedef struct Decoder {
  AVPacket pkt;
  AVPacket pkt_temp;
  PacketQueue* queue;
  AVCodecContext* avctx;
  int pkt_serial;
  int finished;
  int packet_pending;
  SDL_cond* empty_queue_cond;
  int64_t start_pts;
  AVRational start_pts_tb;
  int64_t next_pts;
  AVRational next_pts_tb;
  SDL_Thread* decoder_tid;
} Decoder;

void decoder_init(Decoder* d,
                  AVCodecContext* avctx,
                  PacketQueue* queue,
                  SDL_cond* empty_queue_cond);
int decoder_start(Decoder* d, int (*fn)(void*), void* arg);
void decoder_abort(Decoder* d, FrameQueue* fq);
void decoder_destroy(Decoder* d);
int decoder_decode_frame(Decoder* d, AVFrame* frame, AVSubtitle* sub);
