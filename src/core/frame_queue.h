#pragma once

#include <vector>

#include "ffmpeg_config.h"

extern "C" {
#include <libavcodec/avcodec.h>

#include <SDL2/SDL_render.h>
}

#include "core/packet_queue.h"
#include "core/ring_buffer.h"
#include "core/audio_frame.h"
#include "core/video_frame.h"
#include "core/subtitle_frame.h"

/* no AV correction is done if too big error */
#define SUBPICTURE_QUEUE_SIZE 16
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SAMPLE_QUEUE_SIZE 9

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
  Frame queue[VIDEO_PICTURE_QUEUE_SIZE];

  size_t rindex_;
  size_t rindex_shown_;
  size_t windex_;
  size_t size_;
  const size_t max_size_;
  const bool keep_last_;
  const PacketQueue* const pktq_;
};

template <size_t buffer_size>
class AudioFrameQueue : public RingBuffer<AudioFrame, buffer_size> {
 public:
  typedef RingBuffer<AudioFrame, buffer_size> base_class;
  typedef typename base_class::pointer_type pointer_type;

  AudioFrameQueue(bool keep_last) : base_class(keep_last) {}

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

template <size_t buffer_size>
class SubTitleQueue : public RingBuffer<SubtitleFrame, buffer_size> {
 public:
  typedef RingBuffer<SubtitleFrame, buffer_size> base_class;
  typedef typename base_class::pointer_type pointer_type;

  SubTitleQueue(bool keep_last) : base_class(keep_last) {}

  bool GetFewFrames(std::vector<SubtitleFrame*>* vect, size_t count) {
    if (!vect || count == 0 || count > buffer_size) {
      return false;
    }

    typename base_class::lock_t lock(base_class::queue_mutex_);
    for (size_t i = 0; i < count; ++i) {
      size_t ind = base_class::SafeRindexInner(i);
      SubtitleFrame* fr = base_class::IndexElementInner(ind);
      if (ind + 1 < buffer_size) {
        vect->push_back(fr);
      } else {
        vect->push_back(nullptr);
      }
    }
    return true;
  }

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
