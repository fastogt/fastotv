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

#include <common/logger.h>         // for COMPACT_LOG_FILE_CRIT
#include <common/macros.h>         // for DCHECK
#include <common/threads/types.h>  // for condition_variable, mutex
#include <common/types.h>

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
namespace frames {

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

  bool IsStoped() {
    lock_t lock(queue_mutex_);
    return stoped_;
  }

  pointer_type GetPeekReadable() {
    /* wait until we have a readable a new frame */
    {
      lock_t lock(queue_mutex_);
      while (IsEmptyInner() && !stoped_) {
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
      while (IsFullInner() && !stoped_) {
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

  pointer_type PeekLast() { return RindexElementInner(); }

  pointer_type Peek() { return PeekInner(); }

  pointer_type PeekNextOrNull() {
    if (IsEmptyInner()) {
      return nullptr;
    }
    return PeekNextInner();
  }

  pointer_type Windex() { return WindexElementInner(); }

  bool IsEmpty() const { return IsEmptyInner(); }

  size_t RindexShown() const { return RindexShownInner(); }

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

 private:
  typedef common::unique_lock<common::mutex> lock_t;
  common::condition_variable queue_cond_;
  common::mutex queue_mutex_;

  pointer_type queue_[buffer_size];
  size_t rindex_shown_;
  const bool keep_last_;
  size_t rindex_;
  common::atomic<size_t> windex_;
  common::atomic<size_t> size_;
  bool stoped_;
};

}
}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
