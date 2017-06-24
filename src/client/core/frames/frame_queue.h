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

#include <stdint.h>  // for int64_t

#include "client/core/frames/ring_buffer.h"  // for RingBuffer

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
namespace frames {

struct AudioFrame;
struct VideoFrame;

template <typename T, size_t buffer_size>
class BaseFrameQueue : public RingBuffer<T, buffer_size> {
 public:
  typedef RingBuffer<T, buffer_size> base_class;
  typedef typename base_class::pointer_type pointer_type;

  void Pop() {
    pointer_type fp = base_class::MoveToNext();
    if (!fp) {
      return;
    }
    fp->ClearFrame();
    base_class::RindexUpInner();
    base_class::Signal();
  }

  int64_t GetLastPos() const {
    pointer_type fp = base_class::PeekLast();
    if (base_class::RindexShown()) {
      return fp->pos;
    }

    return 0;
  }
};

template <size_t buffer_size>
class VideoFrameQueue : public BaseFrameQueue<frames::VideoFrame, buffer_size> {
 public:
  typedef BaseFrameQueue<frames::VideoFrame, buffer_size> base_class;
  typedef typename base_class::pointer_type pointer_type;
};

template <size_t buffer_size>
class AudioFrameQueue : public BaseFrameQueue<frames::AudioFrame, buffer_size> {
 public:
  typedef BaseFrameQueue<frames::AudioFrame, buffer_size> base_class;
  typedef typename base_class::pointer_type pointer_type;
};

}  // namespace frames
}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
