#pragma once

#include <stdint.h>  // for int64_t

extern "C" {
#include <libavutil/samplefmt.h>
}

namespace fasto {
namespace fastotv {
namespace client {
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
}
}
}
