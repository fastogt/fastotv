#include "core/types.h"

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/time.h>
}

#include <common/macros.h>

namespace core {

bool IsValidClock(clock_t clock) {
  bool res = !std::isnan(clock);
  return res;
}

clock_t GetRealClokTime() {
  return av_gettime_relative() / 1000000.0;
}

int64_t get_valid_channel_layout(int64_t channel_layout, int channels) {
  if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels) {
    return channel_layout;
  }
  return 0;
}

}  // namespace core
