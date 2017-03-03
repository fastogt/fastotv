#include "core/stream.h"

#include <stddef.h>  // for NULL
#include <atomic>    // for atomic

extern "C" {
#include <libavutil/rational.h>  // for av_q2d
}

#include "core/clock.h"
#include "core/packet_queue.h"

#define MIN_FRAMES 25

namespace core {

Stream::Stream() : packet_queue_(nullptr), clock_(nullptr), stream_index_(-1), stream_st_(NULL) {
  common::atomic<int>* ext_serial = NULL;
  packet_queue_ = PacketQueue::MakePacketQueue(&ext_serial);
  clock_ = new Clock(*ext_serial);
}

Stream::Stream(int index, AVStream* av_stream_st)
    : packet_queue_(nullptr), clock_(nullptr), stream_index_(index), stream_st_(av_stream_st) {}

bool Stream::Open(int index, AVStream* av_stream_st) {
  stream_index_ = index;
  stream_st_ = av_stream_st;
  return IsOpened();
}

bool Stream::IsOpened() const {
  return stream_index_ != -1 && stream_st_ != NULL;
}

void Stream::Close() {
  stream_index_ = -1;
  stream_st_ = NULL;
}

bool Stream::HasEnoughPackets() const {
  if (stream_index_ < 0) {
    return false;
  }

  if (packet_queue_->AbortRequest()) {
    return false;
  }

  bool attach = stream_st_->disposition & AV_DISPOSITION_ATTACHED_PIC;
  if (!attach) {
    return false;
  }

  return (packet_queue_->NbPackets() > MIN_FRAMES &&
          (!packet_queue_->Duration() || q2d() * packet_queue_->Duration() > 1.0));
}

Stream::~Stream() {
  stream_index_ = -1;
  stream_st_ = NULL;
  destroy(&clock_);
  destroy(&packet_queue_);
}

int Stream::Index() const {
  return stream_index_;
}

AVStream* Stream::AvStream() const {
  return stream_st_;
}

double Stream::q2d() const {
  return av_q2d(stream_st_->time_base);
}

clock_t Stream::GetClock() const {
  return clock_->GetClock();
}

void Stream::SetClockAt(clock_t pts, int serial, clock_t time) {
  clock_->SetClockAt(pts, serial, time);
}

void Stream::SetClock(clock_t pts, int serial) {
  clock_->SetClock(pts, serial);
}

void Stream::SetPaused(bool pause) {
  clock_->SetPaused(pause);
}

clock_t Stream::LastUpdatedClock() const {
  return clock_->LastUpdated();
}

void Stream::SyncSerialClock() {
  SetClock(clock_->GetClock(), clock_->Serial());
}

int Stream::Serial() const {
  return clock_->Serial();
}

PacketQueue* Stream::Queue() const {
  return packet_queue_;
}

VideoStream::VideoStream() : Stream() {}

VideoStream::VideoStream(int index, AVStream* av_stream_st) : Stream(index, av_stream_st) {}

AudioStream::AudioStream() : Stream() {}

AudioStream::AudioStream(int index, AVStream* av_stream_st) : Stream(index, av_stream_st) {}

}  // namespace core
