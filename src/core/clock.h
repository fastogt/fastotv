#pragma once

class Clock {
 public:
  explicit Clock(int* queue_serial);

  static void SyncClockToSlave(Clock* c, Clock* slave, double no_sync_threshold);
  void SetClockSpeed(double speed);
  void SetClockAt(double pts, int serial, double time);
  void SetClock(double pts, int serial);
  double GetClock() const;

  double Speed() const;
  int Serial() const;
  double LastUpdated() const;
  double Pts() const;

  bool Paused() const;
  void SetPaused(bool paused);

 private:
  bool paused_;
  double pts_;       /* clock base */
  double pts_drift_; /* clock base minus time at which we updated the clock */
  double last_updated_;
  double speed_;
  int serial_; /* clock is based on a packet with this serial */
  const int* const
      queue_serial_; /* pointer to the current packet queue serial, used for obsolete clock
           detection */
};
