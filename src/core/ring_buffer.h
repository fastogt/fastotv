#pragma once

#include <common/macros.h>
#include <common/thread/types.h>

template <typename T, size_t buffer_size>
class RingBuffer {
 public:
  typedef T* pointer_type;

  RingBuffer(bool keep_last)
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

  pointer_type IndexElementInner(size_t pos) const { return queue_[pos]; }

  size_t SafeRindexInner(size_t pos) const { return (rindex_ + rindex_shown_ + pos) % buffer_size; }

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

  size_t rindex_shown_;
  const bool keep_last_;

 private:
  size_t rindex_;
  size_t windex_;
  size_t size_;
  bool stoped_;
};
