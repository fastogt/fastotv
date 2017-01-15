#pragma once

class Clock {
 public:
  explicit Clock(int* queue_serial);

  static void sync_clock_to_slave(Clock* c, Clock* slave, double no_sync_threshold);
  void set_clock_speed(double speed);
  void set_clock_at(double pts, int serial, double time);
  void set_clock(double pts, int serial);
  double get_clock() const;

  double speed() const;
  int serial() const;
  double last_updated() const;
  double pts() const;

  int paused() const;
  void set_paused(int paused);

 private:
  int paused_;
  double pts_;       /* clock base */
  double pts_drift_; /* clock base minus time at which we updated the clock */
  double last_updated_;
  double speed_;
  int serial_;        /* clock is based on a packet with this serial */
  int* queue_serial_; /* pointer to the current packet queue serial, used for obsolete clock
                        detection */
};
