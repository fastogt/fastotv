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

struct AudioFrame {
  AudioFrame();
  ~AudioFrame();

  AVFrame* frame;
  int serial;
  double pts;      /* presentation timestamp for the frame */
  double duration; /* estimated duration of the frame */
  int64_t pos;     /* byte position of the frame in the input file */

  void ClearFrame();

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioFrame);
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

template <typename T, size_t buffer_size>
class RingBuffer {
 public:
  typedef T* pointer_type;

  RingBuffer(bool keep_last)
      : queue_cond_(),
        queue_mutex_(),
        queue_(),
        rindex_(0),
        rindex_shown_(0),
        windex_(0),
        size_(0),
        keep_last_(keep_last),
        stoped_(false) {
    for (size_t i = 0; i < buffer_size; i++) {
      queue_[i] = new T;
    }
  }

  ~RingBuffer() {
    for (size_t i = 0; i < buffer_size; i++) {
      delete queue_[i];
    }
  }

  T* GetPeekReadable() {
    /* wait until we have a readable a new frame */
    lock_t lock(queue_mutex_);
    while (IsEmptyInner() && !stoped_) {
      queue_cond_.wait(lock);
    }

    if (stoped_) {
      return nullptr;
    }

    return queue_[(rindex_ + rindex_shown_) % buffer_size];
  }

  T* GetPeekWritable() {
    /* wait until we have space to put a new frame */
    lock_t lock(queue_mutex_);
    while (IsFullInner() && !stoped_) {
      queue_cond_.wait(lock);
    }

    if (stoped_) {
      return nullptr;
    }

    return queue_[windex_];
  }

  void Push() {
    lock_t lock(queue_mutex_);
    WindexUpInner();
    queue_cond_.notify_one();
  }

  void Stop() {
    lock_t lock(queue_mutex_);
    stoped_ = true;
    queue_cond_.notify_one();
  }

  T* PeekOrNull() {
    lock_t lock(queue_mutex_);
    if (IsEmptyInner()) {  // if is empty
      return nullptr;
    }
    return queue_[(rindex_ + rindex_shown_) % buffer_size];
  }

  bool IsEmpty() {
    lock_t lock(queue_mutex_);
    return IsEmptyInner();
  }

 protected:
  int NbRemainingInner() const {
    int res = size_ - rindex_shown_;
    DCHECK(res != -1);
    return res;
  }

  bool IsFullInner() const { return size_ >= buffer_size; }

  bool IsEmptyInner() const { return NbRemainingInner() <= 0; }

  pointer_type RindexElementInner() const { return queue_[rindex_]; }

  void RindexUpInner() {
    if (++rindex_ == buffer_size) {
      rindex_ = 0;
    }
    size_--;
  }

  void WindexUpInner() {
    if (++windex_ == buffer_size) {
      windex_ = 0;
    }
    size_++;
  }

  typedef common::thread::unique_lock<common::thread::mutex> lock_t;
  common::thread::condition_variable queue_cond_;
  common::thread::mutex queue_mutex_;

  pointer_type queue_[buffer_size];

  size_t rindex_;
  size_t rindex_shown_;
  size_t windex_;
  size_t size_;

  const bool keep_last_;

 private:
  bool stoped_;
};

template <size_t buffer_size>
class FrameQueueEx : public RingBuffer<AudioFrame, buffer_size> {
 public:
  typedef RingBuffer<AudioFrame, buffer_size> base_class;
  typedef typename base_class::pointer_type pointer_type;

  FrameQueueEx(bool keep_last) : base_class(keep_last) {}

  bool GetLastUsedPos(int64_t* pos, int serial) {
    if (!pos) {
      return false;
    }

    typename base_class::lock_t lock(base_class::queue_mutex_);
    pointer_type fp = base_class::RindexElementInner();
    if (base_class::rindex_shown_ && fp->serial == serial) {
      *pos = fp->pos;
      return true;
    }
    return false;
  }

  void MoveToNext() {
    typename base_class::lock_t lock(base_class::queue_mutex_);
    if (base_class::keep_last_ && !base_class::rindex_shown_) {
      base_class::rindex_shown_ = 1;
      return;
    }
    pointer_type fp = base_class::RindexElementInner();
    fp->ClearFrame();
    base_class::RindexUpInner();
    base_class::queue_cond_.notify_one();
  }
};
