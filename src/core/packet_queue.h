#pragma once

#include "ffmpeg_config.h"

extern "C" {
#include <libavcodec/avcodec.h>

#include <SDL2/SDL_mutex.h>
}

/* no AV correction is done if too big error */
#define SUBPICTURE_QUEUE_SIZE 16
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE \
  FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

typedef struct MyAVPacketList {
  AVPacket pkt;
  struct MyAVPacketList* next;
  int serial;
} MyAVPacketList;

typedef struct PacketQueue {
  MyAVPacketList *first_pkt, *last_pkt;
  int nb_packets;
  int size;
  int64_t duration;
  int abort_request;
  int serial;
  SDL_mutex* mutex;
  SDL_cond* cond;
  static AVPacket* flush_pkt();
} PacketQueue;

/* packet queue handling */
int packet_queue_init(PacketQueue* q);
void packet_queue_flush(PacketQueue* q);
void packet_queue_abort(PacketQueue* q);
int packet_queue_put(PacketQueue* q, AVPacket* pkt);
int packet_queue_put_nullpacket(PacketQueue* q, int stream_index);
/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
int packet_queue_get(PacketQueue* q, AVPacket* pkt, int block, int* serial);
void packet_queue_start(PacketQueue* q);
void packet_queue_destroy(PacketQueue* q);
