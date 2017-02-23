#pragma once

#include <stdint.h>

#include <limits>

namespace core {

typedef uint64_t stream_id;  // must be unique
static const stream_id invalid_stream_id = std::numeric_limits<stream_id>::max();

enum AvSyncType {
  AV_SYNC_AUDIO_MASTER, /* default choice */
  AV_SYNC_VIDEO_MASTER
};

int64_t get_valid_channel_layout(int64_t channel_layout, int channels);
}
