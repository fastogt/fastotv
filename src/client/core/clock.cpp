/*  Copyright (C) 2014-2017 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

#include "client/core/clock.h"

#include <math.h>

extern "C" {
#include <libavutil/time.h>
}

namespace fasto {
namespace fastotv {
namespace client {
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
}
