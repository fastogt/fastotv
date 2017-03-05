#include "core/types.h"

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/time.h>
#include <libavutil/avutil.h>
}

#include <common/macros.h>

namespace core {

bool IsValidClock(clock_t clock) {
  bool res = !std::isnan(clock);
  return res;
}

clock_t GetRealClockTime() {
  return av_gettime_relative() / 1000000.0;
}

msec_t CLockToMsec(clock_t clock) {
  return clock * 1000.0;
}

msec_t GetCurrentMsec() {
  return common::time::current_mstime();
}

bool IsValidMsec(msec_t msec) {
  return msec != invalid_msec;
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
