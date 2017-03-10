#pragma once

#include <stddef.h>  // for size_t

#include <common/logger.h>         // for COMPACT_LOG_FILE_CRIT, etc
#include <common/macros.h>         // for DCHECK
#include <common/threads/types.h>  // for condition_variable, mutex

#include <mutex>  // for unique_lock

namespace fasto {
namespace fastotv {
namespace core {

template <typename T, size_t buffer_size>
class RingBuffer {
 public:
  typedef T* pointer_type;

  explicit RingBuffer(bool keep_last)
      : queue_cond_(),
        queue_mutex_(),
        queue_(),
        rindex_shown_(0),
        keep_last_(keep_last),
        rindex_(0),
        windex_(0),
        size_(0),
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

  template <typename F>
  void ChangeSafeAndNotify(F f, pointer_type el) {
    lock_t lock(queue_mutex_);
    f(el);
    queue_cond_.notify_one();
  }

  template <typename F>
  void WaitSafeAndNotify(F f) {
    lock_t lock(queue_mutex_);
    while (f()) {
      queue_cond_.wait(lock);
    }
  }

  bool IsStoped() {
    lock_t lock(queue_mutex_);
    return stoped_;
  }

  pointer_type GetPeekReadable() {
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

  pointer_type GetPeekWritable() {
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

  pointer_type PeekLast() {
    lock_t lock(queue_mutex_);
    return RindexElementInner();
  }

  pointer_type Peek() {
    lock_t lock(queue_mutex_);
    return PeekInner();
  }

  pointer_type PeekNextOrNull() {
    lock_t lock(queue_mutex_);
    if (IsEmptyInner()) {
      return nullptr;
    }
    return PeekNextInner();
  }

  pointer_type Windex() {
    lock_t lock(queue_mutex_);
    return WindexElementInner();
  }

  bool IsEmpty() {
    lock_t lock(queue_mutex_);
    return IsEmptyInner();
  }

  int NbRemaining() {
    lock_t lock(queue_mutex_);
    return NbRemainingInner();
  }

  size_t RindexShown() {
    lock_t lock(queue_mutex_);
    return RindexShownInner();
  }

 protected:
  int NbRemainingInner() const {
    int res = size_ - rindex_shown_;
    DCHECK(res != -1);
    return res;
  }

  bool IsFullInner() const { return size_ >= buffer_size; }

  bool IsEmptyInner() const { return NbRemainingInner() <= 0; }

  pointer_type PeekInner() { return queue_[(rindex_ + rindex_shown_) % buffer_size]; }

  pointer_type PeekNextInner() { return queue_[(rindex_ + rindex_shown_ + 1) % buffer_size]; }

  pointer_type RindexElementInner() const { return queue_[rindex_]; }

  pointer_type WindexElementInner() { return queue_[windex_]; }

  pointer_type MoveToNextInner() {
    if (keep_last_ && !rindex_shown_) {
      rindex_shown_ = 1;
      return nullptr;
    }

    return RindexElementInner();
  }

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

  size_t RindexShownInner() const { return rindex_shown_; }

  typedef common::unique_lock<common::mutex> lock_t;
  common::condition_variable queue_cond_;
  common::mutex queue_mutex_;

 private:
  pointer_type queue_[buffer_size];
  size_t rindex_shown_;
  const bool keep_last_;
  size_t rindex_;
  size_t windex_;
  size_t size_;
  bool stoped_;
};

}  // namespace core
}
}
