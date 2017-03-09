#include "core/clock.h"

#include <math.h>

extern "C" {
#include <libavutil/time.h>
}

namespace fasto {
namespace fastotv {
namespace core {

Clock::Clock(const common::atomic<serial_id_t>& queue_serial)
    : paused_(false), speed_(1.0), queue_serial_(queue_serial) {
  SetClock(invalid_clock(), invalid_serial_id);
}

void Clock::SetClockAt(clock_t pts, serial_id_t serial, clock_t time) {
  pts_ = pts;
  last_updated_ = time;
  pts_drift_ = pts - time;
  serial_ = serial;
}

void Clock::SetClock(clock_t pts, serial_id_t serial) {
  clock_t time = GetRealClockTime();
  SetClockAt(pts, serial, time);
}

clock_t Clock::GetClock() const {
  if (queue_serial_ != serial_) {
    return invalid_clock();
  }
  if (paused_) {
    return pts_;
  }

  clock_t time = GetRealClockTime();
  return pts_drift_ + time - (time - last_updated_) * (1.0 - speed_);
}

serial_id_t Clock::Serial() const {
  return serial_;
}

clock_t Clock::LastUpdated() const {
  return last_updated_;
}

void Clock::SetPaused(bool paused) {
  paused_ = paused;
}

}  // namespace core
}
}
