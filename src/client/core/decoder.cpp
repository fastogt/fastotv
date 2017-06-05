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

#include "client/core/decoder.h"

#include <errno.h>   // for EAGAIN
#include <stddef.h>  // for NULL

extern "C" {
#include <libavutil/error.h>        // for AVERROR, AVERROR_EOF
#include <libavutil/mathematics.h>  // for av_rescale_q
}

#include <common/logger.h>  // for COMPACT_LOG_ERROR, COMPACT_LOG...
#include <common/macros.h>  // for CHECK, NOTREACHED

#include "client/core/packet_queue.h"  // for PacketQueue

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

Decoder::Decoder(AVCodecContext* avctx, PacketQueue* queue) : avctx_(avctx), queue_(queue), finished_(false) {
  CHECK(queue);
}

void Decoder::Start() {
  queue_->Start();
}

void Decoder::Abort() {
  queue_->Abort();
  queue_->Flush();
}

Decoder::~Decoder() {
  avcodec_free_context(&avctx_);
}

bool Decoder::IsFinished() const {
  return finished_;
}

void Decoder::SetFinished(bool finished) {
  finished_ = finished;
}

AVMediaType Decoder::GetCodecType() const {
  return avctx_->codec_type;
}

AVCodecContext* Decoder::GetAvCtx() const {
  return avctx_;
}

void Decoder::Flush() {
  queue_->Flush();
  avcodec_flush_buffers(avctx_);
}

IFrameDecoder::IFrameDecoder(AVCodecContext* avctx, PacketQueue* queue) : Decoder(avctx, queue) {}

AudioDecoder::AudioDecoder(AVCodecContext* avctx, PacketQueue* queue)
    : IFrameDecoder(avctx, queue), start_pts_(invalid_pts()), start_pts_tb_{0, 0} {
  CHECK(GetCodecType() == AVMEDIA_TYPE_AUDIO);
}

void AudioDecoder::SetStartPts(int64_t start_pts, AVRational start_pts_tb) {
  start_pts_ = start_pts;
  start_pts_tb_ = start_pts_tb;
}

int AudioDecoder::DecodeFrame(AVFrame* frame) {
  int got_frame = 0;
  do {
    AVPacket packet;
    if (!queue_->Get(&packet)) {
      return -1;
    }

    if (packet.data == NULL) {  // flush packet
      SetFinished(false);
      Flush();
      return 0;
    }

    int retcd = avcodec_send_packet(avctx_, &packet);
    if (retcd < 0) {
      if (retcd == AVERROR(EAGAIN)) {
        goto read;
      } else if (retcd == AVERROR_EOF) {
        return 0;
      }
      ERROR_LOG() << "audio avcodec_send_packet error: " << retcd;
      return 0;
    }

  read:
    retcd = avcodec_receive_frame(avctx_, frame);
    if (retcd < 0) {
      if (retcd == AVERROR(EAGAIN)) {
        continue;
      } else if (retcd == AVERROR_EOF) {
        return 0;
      }
      ERROR_LOG() << "audio avcodec_receive_frame error: " << retcd;
      return 0;
    }

    AVRational tb = {1, frame->sample_rate};
    if (IsValidPts(frame->pts)) {
      AVRational stb = av_codec_get_pkt_timebase(avctx_);
      frame->pts = av_rescale_q(frame->pts, stb, tb);
    } else {
      NOTREACHED();
    }
    got_frame = 1;
  } while (!got_frame && !IsFinished());

  return got_frame;
}

VideoDecoder::VideoDecoder(AVCodecContext* avctx, PacketQueue* queue) : IFrameDecoder(avctx, queue) {
  CHECK(GetCodecType() == AVMEDIA_TYPE_VIDEO);
}

int VideoDecoder::GetWidth() const {
  return avctx_->width;
}

int VideoDecoder::GetHeight() const {
  return avctx_->height;
}

int VideoDecoder::DecodeFrame(AVFrame* frame) {
  int got_frame = 0;
  do {
    AVPacket packet;
    if (!queue_->Get(&packet)) {
      return -1;
    }

    if (packet.data == NULL) {  // flush packet
      SetFinished(false);
      Flush();
      return 0;
    }

    int retcd = avcodec_send_packet(avctx_, &packet);
    if (retcd < 0) {
      if (retcd == AVERROR(EAGAIN)) {
        goto read;
      } else if (retcd == AVERROR_EOF) {
        return 0;
      }
      ERROR_LOG() << "video avcodec_send_packet error: " << retcd;
      return -1;
    }

  read:
    retcd = avcodec_receive_frame(avctx_, frame);
    if (retcd < 0) {
      if (retcd == AVERROR(EAGAIN)) {
        continue;
      } else if (retcd == AVERROR_EOF) {
        return 0;
      }
      ERROR_LOG() << "video avcodec_receive_frame error: " << retcd;
      return -1;
    }

    frame->pts = av_frame_get_best_effort_timestamp(frame);
    got_frame = 1;
  } while (!got_frame && !IsFinished());

  return got_frame;
}

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
