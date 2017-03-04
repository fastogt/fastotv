#pragma once

#include <math.h>
#include <stdint.h>

#include <limits>

namespace core {

typedef double clock_t;
static const clock_t invalid_clock = NAN;

bool IsValidClock(clock_t clock);
clock_t GetRealClockTime();  // sec

typedef int serial_id_t;
static const serial_id_t invalid_serial_id = -1;

typedef uint64_t stream_id;  // must be unique
static const stream_id invalid_stream_id = std::numeric_limits<stream_id>::max();

enum AvSyncType {
  AV_SYNC_AUDIO_MASTER, /* default choice */
  AV_SYNC_VIDEO_MASTER
};

int64_t get_valid_channel_layout(int64_t channel_layout, int channels);
}
