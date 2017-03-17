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

#pragma once

#include <math.h>

#include <common/types.h>

#include "client/core/types.h"

namespace fasto {
namespace fastotv {
namespace client {
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
}
