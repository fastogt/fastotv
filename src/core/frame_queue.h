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
struct Frame {
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
};

class FrameQueue {
 public:
  FrameQueue(PacketQueue* pktq, size_t max_size, bool keep_last);
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
  size_t rindexShown() const;
  size_t windex() const;

  SDL_mutex* mutex;
  SDL_cond* cond;
  Frame queue[FRAME_QUEUE_SIZE];

 private:
  size_t rindex_;
  size_t rindex_shown_;
  size_t windex_;
  size_t size_;
  const size_t max_size_;
  const bool keep_last_;
  const PacketQueue* const pktq_;
};
