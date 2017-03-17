#pragma once

#include <stddef.h>  // for size_t
#include <stdint.h>  // for int64_t

#include "client/core/ring_buffer.h"
#include "client/core/audio_frame.h"
#include "client/core/video_frame.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

template <size_t buffer_size>
class VideoFrameQueue : public RingBuffer<VideoFrame, buffer_size> {
 public:
  typedef RingBuffer<VideoFrame, buffer_size> base_class;
  typedef typename base_class::pointer_type pointer_type;

  explicit VideoFrameQueue(bool keep_last) : base_class(keep_last) {}

  bool GetLastUsedPos(int64_t* pos, int serial) {
    if (!pos) {
      return false;
    }

    typename base_class::lock_t lock(base_class::queue_mutex_);
    pointer_type fp = base_class::RindexElementInner();
    if (base_class::RindexShownInner() && fp->serial == serial) {
      *pos = fp->pos;
      return true;
    }
    return false;
  }

  void MoveToNext() {
    typename base_class::lock_t lock(base_class::queue_mutex_);
    pointer_type fp = base_class::MoveToNextInner();
    if (!fp) {
      return;
    }
    fp->ClearFrame();
    base_class::RindexUpInner();
    base_class::queue_cond_.notify_one();
  }
};

template <size_t buffer_size>
class AudioFrameQueue : public RingBuffer<AudioFrame, buffer_size> {
 public:
  typedef RingBuffer<AudioFrame, buffer_size> base_class;
  typedef typename base_class::pointer_type pointer_type;

  explicit AudioFrameQueue(bool keep_last) : base_class(keep_last) {}

  bool GetLastUsedPos(int64_t* pos, int serial) {
    if (!pos) {
      return false;
    }

    typename base_class::lock_t lock(base_class::queue_mutex_);
    pointer_type fp = base_class::RindexElementInner();
    if (base_class::RindexShownInner() && fp->serial == serial) {
      *pos = fp->pos;
      return true;
    }
    return false;
  }

  void MoveToNext() {
    typename base_class::lock_t lock(base_class::queue_mutex_);
    pointer_type fp = base_class::MoveToNextInner();
    if (!fp) {
      return;
    }
    fp->ClearFrame();
    base_class::RindexUpInner();
    base_class::queue_cond_.notify_one();
  }
};

}  // namespace core
}
}
}
