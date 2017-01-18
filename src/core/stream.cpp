#include "core/stream.h"

#include "core/clock.h"

#define MIN_FRAMES 25

Stream::Stream() : packet_queue_(nullptr), clock_(nullptr), stream_index_(-1), stream_st_(NULL) {
  int* ext_serial = NULL;
  packet_queue_ = PacketQueue::make_packet_queue(&ext_serial);
  clock_ = new Clock(ext_serial);
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

int Stream::HasEnoughPackets() {
  return stream_index_ < 0 || packet_queue_->abort_request() ||
         (stream_st_->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
         (packet_queue_->nb_packets() > MIN_FRAMES &&
          (!packet_queue_->duration() || q2d() * packet_queue_->duration() > 1.0));
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

double Stream::GetClock() const {
  return clock_->get_clock();
}

double Stream::GetPts() const {
  return clock_->pts();
}

void Stream::SetClockSpeed(double speed) {
  clock_->set_clock_speed(speed);
}

double Stream::GetSpeed() const {
  return clock_->speed();
}

void Stream::SetClockAt(double pts, int serial, double time) {
  clock_->set_clock_at(pts, serial, time);
}

void Stream::SetClock(double pts, int serial) {
  clock_->set_clock(pts, serial);
}

void Stream::SetPaused(bool pause) {
  clock_->set_paused(pause);
}

double Stream::LastUpdatedClock() const {
  return clock_->last_updated();
}

void Stream::SyncSerialClock() {
  SetClock(clock_->get_clock(), clock_->serial());
}

int Stream::Serial() const {
  return clock_->serial();
}

void Stream::SyncClockWith(Stream* str, double no_sync_threshold) {
  Clock::sync_clock_to_slave(clock_, str->clock_, no_sync_threshold);
}

PacketQueue* Stream::Queue() const {
  return packet_queue_;
}

VideoStream::VideoStream() : Stream() {}

VideoStream::VideoStream(int index, AVStream* av_stream_st) : Stream(index, av_stream_st) {}

AudioStream::AudioStream() : Stream() {}

AudioStream::AudioStream(int index, AVStream* av_stream_st) : Stream(index, av_stream_st) {}

SubtitleStream::SubtitleStream() : Stream() {}

SubtitleStream::SubtitleStream(int index, AVStream* av_stream_st) : Stream(index, av_stream_st) {}
