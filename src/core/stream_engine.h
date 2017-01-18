#pragma once

#include "core/packet_queue.h"
#include "core/frame_queue.h"

class Clock;
class StreamEngine {
 public:
  StreamEngine(size_t max_size, bool keep_last);
  ~StreamEngine();

  FrameQueue* frame_queue_;
};
