#include "core/types.h"

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/time.h>
#include <libavutil/avutil.h>
}

#include <common/macros.h>

namespace fasto {
namespace fastotv {
namespace core {

clock_t invalid_clock() {
  return -1;
}

bool IsValidClock(clock_t clock) {
  return clock != invalid_clock();
}

clock_t GetRealClockTime() {
  return GetCurrentMsec();
}

msec_t ClockToMsec(clock_t clock) {
  return clock;
}

msec_t GetCurrentMsec() {
  return common::time::current_mstime();
}

int64_t get_valid_channel_layout(int64_t channel_layout, int channels) {
  if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels) {
    return channel_layout;
  }
  return 0;
}

pts_t invalid_pts() {
  return AV_NOPTS_VALUE;
}

bool IsValidPts(pts_t pts) {
  return pts != invalid_pts();
}

}  // namespace core
}
}
