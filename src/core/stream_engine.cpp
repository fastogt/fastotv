#include "core/stream_engine.h"

#include <common/macros.h>

#include "core/clock.h"

StreamEngine::StreamEngine(size_t max_size, bool keep_last)
    : frame_queue_(nullptr) {

}

StreamEngine::~StreamEngine() {
  destroy(&frame_queue_);
}
