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

#include "client/player/core/stream.h"

#include <stddef.h>  // for NULL

#include <common/logger.h>  // for COMPACT_LOG_FILE_CRIT
#include <common/time.h>    // for current_mstime

#include "client/player/core/av_utils.h"
#include "client/player/core/clock.h"         // for Clock
#include "client/player/core/packet_queue.h"  // for PacketQueue

#include "client/types.h"  // for Size

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

Stream::Stream()
    : stream_st_(NULL),
      packet_queue_(new PacketQueue),
      clock_(new Clock),
      stream_index_(-1),
      bandwidth_(),
      start_ts_(0),
      total_downloaded_bytes_(0) {}

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
  if (!IsOpened()) {
    return false;
  }

  if (packet_queue_->IsAborted()) {
    return false;
  }

  bool attach = stream_st_->disposition & AV_DISPOSITION_ATTACHED_PIC;
  if (!attach) {
    return false;
  }

  return (packet_queue_->GetNbPackets() > minimum_frames &&
          (!packet_queue_->GetDuration() || q2d() * packet_queue_->GetDuration() > 1000));
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

AVRational Stream::GetTimeBase() const {
  return stream_st_ ? stream_st_->time_base : AVRational();
}

AVCodecParameters* Stream::GetCodecpar() const {
  return stream_st_ ? stream_st_->codecpar : NULL;
}

double Stream::q2d() const {
  return q2d_diff(stream_st_->time_base);
}

clock64_t Stream::GetPts() const {
  return clock_->GetPts();
}

clock64_t Stream::GetClock() const {
  return clock_->GetClock();
}

void Stream::SetClockAt(clock64_t pts, clock64_t time) {
  clock_->SetClockAt(pts, time);
}

void Stream::SetClock(clock64_t pts) {
  clock_->SetClock(pts);
}

void Stream::SetPaused(bool pause) {
  clock_->SetPaused(pause);

  total_downloaded_bytes_ = 0;
  start_ts_ = 0;
}

clock64_t Stream::LastUpdatedClock() const {
  return clock_->LastUpdated();
}

void Stream::SyncSerialClock() {
  SetClock(clock_->GetClock());
}

PacketQueue* Stream::GetQueue() const {
  return packet_queue_;
}

void Stream::RegisterPacket(const AVPacket* packet) {
  if (!packet || packet->size < 0) {
    return;
  }

  unsigned int packet_size = static_cast<unsigned int>(packet->size);
  common::time64_t cur_ts = common::time::current_mstime();
  if (total_downloaded_bytes_ == 0) {
    start_ts_ = cur_ts;
  }
  total_downloaded_bytes_ += packet_size;
}

bandwidth_t Stream::Bandwidth() const {
  common::time64_t cur_ts = common::time::current_mstime();
  const common::time64_t data_interval = cur_ts - start_ts_;
  return CalculateBandwidth(total_downloaded_bytes_, data_interval);
}

size_t Stream::TotalDownloadedBytes() const {
  return total_downloaded_bytes_;
}

DesireBytesPerSec Stream::DesireBandwith() const {
  return bandwidth_;
}

void Stream::SetDesireBandwith(const DesireBytesPerSec& band) {
  bandwidth_ = band;
}

VideoStream::VideoStream() : Stream(), frame_rate_() {}

bool VideoStream::Open(int index, AVStream* av_stream_st, AVRational frame_rate) {
  AVCodecParameters* codecpar = av_stream_st->codecpar;
  DesireBytesPerSec band;
  if (codecpar->bit_rate != 0) {
    band = VideoBitrateAverage(codecpar->bit_rate / 8);
  } else {
    Size frame_size(codecpar->width, codecpar->height);
    int profile = codecpar->profile;
    if (codecpar->codec_id == AV_CODEC_ID_H264) {
      band = CalculateDesireH264BandwidthBytesPerSec(frame_size, av_q2d(frame_rate), profile);
    } else if (codecpar->codec_id == AV_CODEC_ID_MPEG2TS) {
      band = CalculateDesireMPEGBandwidthBytesPerSec(frame_size);
    } else {
      NOTREACHED();
    }
  }

  SetDesireBandwith(band);
  frame_rate_ = frame_rate;
  return Stream::Open(index, av_stream_st);
}

AVRational VideoStream::GetFrameRate() const {
  return frame_rate_;
}

double VideoStream::GetRotation() const {
  return get_rotation(stream_st_);
}

bool VideoStream::HaveDispositionPicture() const {
  return stream_st_->disposition & AV_DISPOSITION_ATTACHED_PIC;
}

AVRational VideoStream::GetAspectRatio() const {
  AVRational undef = {0, 1};
  return stream_st_ ? stream_st_->sample_aspect_ratio : undef;
}

AVRational VideoStream::StableAspectRatio(AVFrame* frame) const {
  return guess_sample_aspect_ratio(stream_st_, frame);
}

AudioStream::AudioStream() : Stream() {}

bool AudioStream::Open(int index, AVStream* av_stream_st) {
  AVCodecParameters* codecpar = av_stream_st->codecpar;
  DesireBytesPerSec band;
  if (codecpar->bit_rate != 0) {
    band = AudioBitrateAverage(codecpar->bit_rate / 8);
  } else {
    if (codecpar->codec_id == AV_CODEC_ID_AAC || codecpar->codec_id == AV_CODEC_ID_AAC_LATM) {
      band = CalculateDesireAACBandwidthBytesPerSec(codecpar->channels);
    } else if (codecpar->codec_id == AV_CODEC_ID_MP2) {
      band = CalculateDesireMP2BandwidthBytesPerSec(codecpar->channels);
    } else {
      NOTREACHED();
    }
  }

  SetDesireBandwith(band);
  return Stream::Open(index, av_stream_st);
}

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
