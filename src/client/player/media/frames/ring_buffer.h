/*  Copyright (C) 2014-2017 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

#include <common/logger.h>  // for COMPACT_LOG_FILE_CRIT
#include <common/macros.h>  // for DCHECK
#include <common/types.h>

namespace fastotv {
namespace client {
namespace player {
namespace media {
namespace frames {

template <typename T, size_t buffer_size>
class RingBuffer {
 public:
  typedef T* pointer_type;

  RingBuffer()
      : queue_cond_(), queue_mutex_(), queue_(), rindex_shown_(0), rindex_(0), windex_(0), size_(0), stoped_(false) {
    for (size_t i = 0; i < buffer_size; i++) {
      queue_[i] = new T;
    }
  }

  ~RingBuffer() {
    for (size_t i = 0; i < buffer_size; i++) {
      delete queue_[i];
    }
  }

  bool IsStoped() {
    lock_t lock(queue_mutex_);
    return stoped_;
  }

  pointer_type GetPeekReadable() {
    /* wait until we have a readable a new frame */
    {
      lock_t lock(queue_mutex_);
      while (IsEmpty() && !stoped_) {
        queue_cond_.wait(lock);
      }

      if (stoped_) {
        return nullptr;
      }
    }

    return queue_[(rindex_ + rindex_shown_) % buffer_size];
  }

  pointer_type GetPeekWritable() {
    /* wait until we have space to put a new frame */
    {
      lock_t lock(queue_mutex_);
      while (IsFull() && !stoped_) {
        queue_cond_.wait(lock);
      }

      if (stoped_) {
        return nullptr;
      }
    }

    return queue_[windex_];
  }

  void Push() {
    WindexUpInner();
    Signal();
  }

  void Signal() {
    lock_t lock(queue_mutex_);
    queue_cond_.notify_one();
  }

  void Stop() {
    lock_t lock(queue_mutex_);
    stoped_ = true;
    queue_cond_.notify_one();
  }

  pointer_type PeekLast() const { return queue_[rindex_]; }

  pointer_type Peek() const { return queue_[(rindex_ + rindex_shown_) % buffer_size]; }

  pointer_type PeekNextOrNull() const {
    if (IsEmpty()) {
      return nullptr;
    }
    return queue_[(rindex_ + rindex_shown_ + 1) % buffer_size];
  }

  bool IsEmpty() const { return size_ - rindex_shown_ <= 0; }
  bool IsFull() const { return size_ >= buffer_size; }

  size_t RindexShown() const { return rindex_shown_; }

 protected:
  pointer_type MoveToNext() const {
    if (!rindex_shown_) {
      rindex_shown_ = 1;
      return nullptr;
    }

    return PeekLast();
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

 private:
  typedef std::unique_lock<std::mutex> lock_t;
  std::condition_variable queue_cond_;
  std::mutex queue_mutex_;

  pointer_type queue_[buffer_size];
  mutable size_t rindex_shown_;  // in mostly const
  size_t rindex_;
  std::atomic<size_t> windex_;
  std::atomic<size_t> size_;
  bool stoped_;
};

}  // namespace frames
}  // namespace media
}  // namespace player
}  // namespace client
}  // namespace fastotv
