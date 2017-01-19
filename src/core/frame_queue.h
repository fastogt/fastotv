#pragma once

#include <vector>

#include "core/ring_buffer.h"
#include "core/audio_frame.h"
#include "core/video_frame.h"
#include "core/subtitle_frame.h"

template <size_t buffer_size>
class VideoFrameQueueEx : public RingBuffer<VideoFrame, buffer_size> {
 public:
  typedef RingBuffer<VideoFrame, buffer_size> base_class;
  typedef typename base_class::pointer_type pointer_type;

  VideoFrameQueueEx(bool keep_last) : base_class(keep_last) {}

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

  AudioFrameQueue(bool keep_last) : base_class(keep_last) {}

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
