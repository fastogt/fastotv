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

#include <stddef.h>  // for size_t
#include <stdint.h>  // for int64_t

#include "client/core/audio_frame.h"
#include "client/core/ring_buffer.h"
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

  void MoveToNext() {
    pointer_type fp = base_class::MoveToNextInner();
    if (!fp) {
      return;
    }
    fp->ClearFrame();
    base_class::RindexUpInner();
    base_class::Signal();
  }

  int64_t GetLastPos() const {
    pointer_type fp = base_class::RindexElementInner();
    if (base_class::RindexShown()) {
      return fp->pos;
    }

    return -1;
  }
};

template <size_t buffer_size>
class AudioFrameQueue : public RingBuffer<AudioFrame, buffer_size> {
 public:
  typedef RingBuffer<AudioFrame, buffer_size> base_class;
  typedef typename base_class::pointer_type pointer_type;

  explicit AudioFrameQueue(bool keep_last) : base_class(keep_last) {}

  void MoveToNext() {
    pointer_type fp = base_class::MoveToNextInner();
    if (!fp) {
      return;
    }
    fp->ClearFrame();
    base_class::RindexUpInner();
    base_class::Signal();
  }

  int64_t GetLastPos() const {
    pointer_type fp = base_class::RindexElementInner();
    if (base_class::RindexShown()) {
      return fp->pos;
    }

    return -1;
  }
};

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
