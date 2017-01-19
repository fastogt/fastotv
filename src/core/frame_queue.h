#pragma once

#include <vector>

#include "ffmpeg_config.h"

extern "C" {
#include <libavcodec/avcodec.h>

#include <SDL2/SDL_render.h>
}

#include <common/thread/types.h>

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
  size_t rindex_shown() const;
  size_t windex() const;

  SDL_mutex* mutex;
  SDL_cond* cond;

  Frame* windex_frame();

 private:
  Frame queue[FRAME_QUEUE_SIZE];

  size_t rindex_;
  size_t rindex_shown_;
  size_t windex_;
  size_t size_;
  const size_t max_size_;
  const bool keep_last_;
  const PacketQueue* const pktq_;
};

class FrameQueueEx {
 public:
  FrameQueueEx(size_t max_size, bool keep_last);
  ~FrameQueueEx();

  bool GetLastUsedPos(int64_t* pos, int serial);
  Frame* GetPeekReadable();
  Frame* GetPeekWritable();
  Frame* PeekOrNull();  // peek or null if empty queue

  void MoveToNext();
  void Push();
  void Stop();

  bool IsEmpty();

 private:
  bool IsFullInner() const;
  bool IsEmptyInner() const;
  int NbRemainingInner() const;

  typedef common::thread::unique_lock<common::thread::mutex> lock_t;
  common::thread::condition_variable queue_cond_;
  common::thread::mutex queue_mutex_;
  Frame queue_[FRAME_QUEUE_SIZE];

  size_t rindex_;
  size_t rindex_shown_;
  size_t windex_;
  size_t size_;
  const size_t max_size_;
  const bool keep_last_;
  bool stoped_;
};
