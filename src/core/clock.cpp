#include "core/clock.h"

#include <math.h>

extern "C" {
#include <libavutil/time.h>
}

Clock::Clock(int* queue_serial) : paused_(false), speed_(1.0), queue_serial_(queue_serial) {
  set_clock(NAN, -1);
}

void Clock::sync_clock_to_slave(Clock* c, Clock* slave, double no_sync_threshold) {
  double clock = c->get_clock();
  double slave_clock = slave->get_clock();
  if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > no_sync_threshold)) {
    c->set_clock(slave_clock, slave->serial_);
  }
}

void Clock::set_clock_speed(double speed) {
  set_clock(get_clock(), serial_);
  speed_ = speed;
}

void Clock::set_clock_at(double pts, int serial, double time) {
  pts_ = pts;
  last_updated_ = time;
  pts_drift_ = pts - time;
  serial_ = serial;
}

void Clock::set_clock(double pts, int serial) {
  double time = av_gettime_relative() / 1000000.0;
  set_clock_at(pts, serial, time);
}

double Clock::get_clock() const {
  if (*queue_serial_ != serial_) {
    return NAN;
  }
  if (paused_) {
    return pts_;
  }

  double time = av_gettime_relative() / 1000000.0;
  return pts_drift_ + time - (time - last_updated_) * (1.0 - speed_);
}

double Clock::speed() const {
  return speed_;
}

int Clock::serial() const {
  return serial_;
}

double Clock::last_updated() const {
  return last_updated_;
}

double Clock::pts() const {
  return pts_;
}

bool Clock::paused() const {
  return paused_;
}

void Clock::set_paused(bool paused) {
  paused_ = paused;
}
