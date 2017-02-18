#pragma once

#include "ffmpeg_config.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <common/macros.h>

namespace core {
class Clock;
class PacketQueue;
}  // lines 12-12

namespace core {

class Stream {
 public:
  bool HasEnoughPackets() const;
  virtual bool Open(int index, AVStream* av_stream_st);
  bool IsOpened() const;
  virtual void Close();
  virtual ~Stream();

  int Index() const;
  AVStream* AvStream() const;
  double q2d() const;

  // clock interface
  double GetClock() const;

  void SetClockAt(double pts, int serial, double time);
  void SetClock(double pts, int serial);
  void SetPaused(bool pause);

  double LastUpdatedClock() const;

  void SyncSerialClock();

  int Serial() const;

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
