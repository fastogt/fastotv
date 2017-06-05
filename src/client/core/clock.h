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

#include "client/core/types.h"  // for clock_t

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

class Clock {
 public:
  explicit Clock();

  void SetClockAt(clock_t pts, clock_t time);
  void SetClock(clock_t pts);
  clock_t GetClock() const;

  clock_t LastUpdated() const;

  void SetPaused(bool paused);

 private:
  bool paused_;
  clock_t pts_;       /* clock base */
  clock_t pts_drift_; /* clock base minus time at which we updated the clock */
  clock_t last_updated_;
  double speed_;
};
}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
