#pragma once

#include <stdint.h>

extern "C" {
#include <libavutil/samplefmt.h>
}

struct AudioParams {
  int freq;
  int channels;
  int64_t channel_layout;
  enum AVSampleFormat fmt;
  int frame_size;
  int bytes_per_sec;
};
