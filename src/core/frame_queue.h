#pragma once

extern "C" {
#include <libavcodec/avcodec.h>

#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_render.h>
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

typedef struct AudioParams {
  int freq;
  int channels;
  int64_t channel_layout;
  enum AVSampleFormat fmt;
  int frame_size;
  int bytes_per_sec;
} AudioParams;

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct Frame {
  AVFrame* frame;
  AVSubtitle sub;
  int serial;
  double pts;      /* presentation timestamp for the frame */
  double duration; /* estimated duration of the frame */
  int64_t pos;     /* byte position of the frame in the input file */
  SDL_Texture* bmp;
  int allocated;
  int width;
  int height;
  int format;
  AVRational sar;
  int uploaded;
  int flip_v;
} Frame;

void frame_queue_unref_item(Frame* vp);

typedef struct FrameQueue {
  Frame queue[FRAME_QUEUE_SIZE];
  int rindex;
  int windex;
  int size;
  int max_size;
  int keep_last;
  int rindex_shown;
  SDL_mutex* mutex;
  SDL_cond* cond;
  PacketQueue* pktq;
} FrameQueue;

int frame_queue_init(FrameQueue* f, PacketQueue* pktq, int max_size, int keep_last);
void frame_queue_push(FrameQueue* f);
Frame* frame_queue_peek_writable(FrameQueue* f);
int frame_queue_nb_remaining(FrameQueue* f);
Frame* frame_queue_peek_last(FrameQueue* f);
Frame* frame_queue_peek(FrameQueue* f);
Frame* frame_queue_peek_next(FrameQueue* f);
Frame* frame_queue_peek_readable(FrameQueue* f);
void frame_queue_signal(FrameQueue* f);
void frame_queue_next(FrameQueue* f);
int64_t frame_queue_last_pos(FrameQueue* f);
void frame_queue_destory(FrameQueue* f);
