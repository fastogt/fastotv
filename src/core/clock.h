#pragma once

#include <math.h>

#include <common/types.h>

#include "core/types.h"

namespace fasto {
namespace fastotv {
namespace core {

class Clock {
 public:
  explicit Clock(const common::atomic<serial_id_t>& queue_serial);

  void SetClockAt(clock_t pts, serial_id_t serial, clock_t time);
  void SetClock(clock_t pts, serial_id_t serial);
  clock_t GetClock() const;

  serial_id_t Serial() const;
  clock_t LastUpdated() const;

  void SetPaused(bool paused);

 private:
  bool paused_;
  clock_t pts_;       /* clock base */
  clock_t pts_drift_; /* clock base minus time at which we updated the clock */
  clock_t last_updated_;
  double speed_;
  serial_id_t serial_; /* clock is based on a packet with this serial */
  const common::atomic<serial_id_t>&
      queue_serial_; /* pointer to the current packet queue serial, used for obsolete clock
           detection */
};
}
}
}
