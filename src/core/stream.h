#pragma once

#include "ffmpeg_config.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <common/macros.h>

#include "core/types.h"

namespace core {
class Clock;
class PacketQueue;
}  // lines 12-12

namespace core {

class Stream {
 public:
  enum { minimum_frames = 25 };
  bool HasEnoughPackets() const;
  virtual bool Open(int index, AVStream* av_stream_st);
  bool IsOpened() const;
  virtual void Close();
  virtual ~Stream();

  int Index() const;
  AVStream* AvStream() const;
  double q2d() const;

  // clock interface
  clock_t GetClock() const;

  void SetClockAt(clock_t pts, int serial, clock_t time);
  void SetClock(clock_t pts, int serial);
  void SetPaused(bool pause);

  clock_t LastUpdatedClock() const;

  void SyncSerialClock();

  serial_id_t Serial() const;

  PacketQueue* Queue() const;

 protected:
  Stream();
  Stream(int index, AVStream* av_stream_st);

 private:
  DISALLOW_COPY_AND_ASSIGN(Stream);
  PacketQueue* packet_queue_;
  Clock* clock_;
  int stream_index_;
  AVStream* stream_st_;
};

class VideoStream : public Stream {
 public:
  VideoStream();
  VideoStream(int index, AVStream* av_stream_st);
};

class AudioStream : public Stream {
 public:
  AudioStream();
  AudioStream(int index, AVStream* av_stream_st);
};

}  // namespace core
