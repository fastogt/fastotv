#pragma once

#include "core/packet_queue.h"
#include "core/frame_queue.h"

class Clock;
class StreamEngine {
 public:
  StreamEngine(size_t max_size, bool keep_last);
  ~StreamEngine();

  double GetClock() const;
  double GetPts() const;

  void SetClockSpeed(double speed);
  double GetSpeed() const;

  void SetClockAt(double pts, int serial, double time);
  void SetClock(double pts, int serial);
  void SetPaused(bool pause);

  double LastUpdatedClock() const;

  void SyncSerialClock();

  int Serial() const;

  void SyncClockWith(StreamEngine* engine, double no_sync_threshold);

  PacketQueue* packet_queue_;
  FrameQueue* frame_queue_;

 private:
  Clock* clock_;
};
