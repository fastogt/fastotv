/*  Copyright (C) 2014-2017 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "ffmpeg_config.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <common/macros.h>

#include "client/core/bandwidth_estimation.h"
#include "client/core/types.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
class Clock;
class PacketQueue;

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

  void SetClockAt(clock_t pts, clock_t time);
  void SetClock(clock_t pts);
  void SetPaused(bool pause);

  clock_t LastUpdatedClock() const;

  void SyncSerialClock();

  PacketQueue* Queue() const;
  bandwidth_t Bandwidth() const;
  DesireBytesPerSec DesireBandwith() const;
  size_t TotalDownloadedBytes() const;
  void RegisterPacket(const AVPacket* packet);

 protected:
  Stream();
  void SetDesireBandwith(const DesireBytesPerSec& band);

 private:
  DISALLOW_COPY_AND_ASSIGN(Stream);

  PacketQueue* packet_queue_;
  Clock* clock_;
  int stream_index_;
  AVStream* stream_st_;

  DesireBytesPerSec bandwidth_;
  common::time64_t start_ts_;
  size_t total_downloaded_bytes_;
};

class VideoStream : public Stream {
 public:
  VideoStream();

  virtual bool Open(int index, AVStream* av_stream_st, AVRational frame_rate);

 private:
  using Stream::Open;
};

class AudioStream : public Stream {
 public:
  AudioStream();

  virtual bool Open(int index, AVStream* av_stream_st) override;
};

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
