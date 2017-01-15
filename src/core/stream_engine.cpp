#include "core/stream_engine.h"

#include <common/macros.h>

#include "core/clock.h"

StreamEngine::StreamEngine(size_t max_size, bool keep_last)
    : packet_queue_(nullptr), frame_queue_(nullptr), clock_(nullptr) {
  packet_queue_ = new PacketQueue;
  frame_queue_ = new FrameQueue(packet_queue_, max_size, keep_last);
  clock_ = new Clock(&packet_queue_->serial);
}

double StreamEngine::GetClock() const {
  return clock_->get_clock();
}

double StreamEngine::GetPts() const {
  return clock_->pts();
}

void StreamEngine::SetClockSpeed(double speed) {
  clock_->set_clock_speed(speed);
}

double StreamEngine::GetSpeed() const {
  return clock_->speed();
}

void StreamEngine::SetClockAt(double pts, int serial, double time) {
  clock_->set_clock_at(pts, serial, time);
}

void StreamEngine::SetClock(double pts, int serial) {
  clock_->set_clock(pts, serial);
}

void StreamEngine::SetPaused(bool pause) {
  clock_->set_paused(pause);
}

double StreamEngine::LastUpdatedClock() const {
  return clock_->last_updated();
}

void StreamEngine::SyncSerialClock() {
  SetClock(clock_->get_clock(), clock_->serial());
}

int StreamEngine::Serial() const {
  return clock_->serial();
}

void StreamEngine::SyncClockWith(StreamEngine* engine, double no_sync_threshold) {
  Clock::sync_clock_to_slave(clock_, engine->clock_, no_sync_threshold);
}

StreamEngine::~StreamEngine() {
  destroy(&clock_);
  destroy(&frame_queue_);
  destroy(&packet_queue_);
}
