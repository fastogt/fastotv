#pragma once

extern "C" {
#include <libavutil/samplefmt.h>
}

namespace core {

struct AudioParams {
  int freq;
  int channels;
  int64_t channel_layout;
  enum AVSampleFormat fmt;
  int frame_size;
  int bytes_per_sec;
};

}  // namespace core
