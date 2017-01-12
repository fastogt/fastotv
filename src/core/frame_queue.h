#pragma once

#include "ffmpeg_config.h"

extern "C" {
#include <libavcodec/avcodec.h>

#include <SDL2/SDL_render.h>
}

#include "core/packet_queue.h"

/* no AV correction is done if too big error */
#define SUBPICTURE_QUEUE_SIZE 16
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE \
  FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

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

class FrameQueue {
 public:
  FrameQueue(PacketQueue* pktq, int max_size, bool keep_last);
  ~FrameQueue();

  void push();
  Frame* peek_writable();
  int nb_remaining();
  Frame* peek_last();
  Frame* peek();
  Frame* peek_next();
  Frame* peek_readable();
  void signal();
  void next();
  int64_t last_pos();

  SDL_mutex* mutex;
  SDL_cond* cond;
  int rindex_shown;
  Frame queue[FRAME_QUEUE_SIZE];
  int windex;

 private:
  int rindex_;
  int size_;
  int max_size_;
  bool keep_last_;
  PacketQueue* pktq_;
};
