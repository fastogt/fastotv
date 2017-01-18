#pragma once

#include "ffmpeg_config.h"
extern "C" {
#include <libavformat/avformat.h>
}

#include <common/macros.h>

#include "core/packet_queue.h"

class Clock;

class Stream {
 public:
  int HasEnoughPackets();
  virtual bool Open(int index, AVStream* av_stream_st);
  bool IsOpened() const;
  virtual void Close();
  virtual ~Stream();

  int Index() const;
  AVStream* AvStream() const;
  double q2d() const;

  // clock interface
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

  void SyncClockWith(Stream* str, double no_sync_threshold);

  PacketQueue* Queue() const;

 protected:
  Stream();
  Stream(int index, AVStream* av_stream_st);

 private:
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

class SubtitleStream : public Stream {
 public:
  SubtitleStream();
  SubtitleStream(int index, AVStream* av_stream_st);
};
