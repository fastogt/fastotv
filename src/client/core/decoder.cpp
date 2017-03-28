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

extern "C" {
#include <libavutil/mathematics.h>  // for av_rescale_q
}

#include <common/logger.h>  // for COMPACT_LOG_FILE_CRIT, etc
#include <common/macros.h>  // for CHECK

#include "client/core/packet_queue.h"  // for PacketQueu

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

Decoder::Decoder(AVCodecContext* avctx, PacketQueue* queue)
    : avctx_(avctx),
      pkt_(),
      queue_(queue),
      packet_pending_(false),
      finished_(false) {
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
  av_packet_unref(&pkt_);
  avcodec_free_context(&avctx_);
}

bool Decoder::Finished() const {
  return finished_;
}

void Decoder::SetFinished(bool finished) {
  finished_ = finished;
}

AVMediaType Decoder::CodecType() const {
  return avctx_->codec_type;
}

IFrameDecoder::IFrameDecoder(AVCodecContext* avctx, PacketQueue* queue) : Decoder(avctx, queue) {}

AudioDecoder::AudioDecoder(AVCodecContext* avctx, PacketQueue* queue)
    : IFrameDecoder(avctx, queue), start_pts_(invalid_pts()), start_pts_tb_{0, 0} {
  CHECK(CodecType() == AVMEDIA_TYPE_AUDIO);
}

void AudioDecoder::SetStartPts(int64_t start_pts, AVRational start_pts_tb) {
  start_pts_ = start_pts;
  start_pts_tb_ = start_pts_tb;
}

int AudioDecoder::DecodeFrame(AVFrame* frame) {
  int got_frame = 0;
  AVPacket pkt_temp;
  static const AVPacket* fls = PacketQueue::FlushPkt();
  do {
    if (queue_->IsAborted()) {
      return -1;
    }

    if (!packet_pending_) {
      AVPacket lpkt;
      do {
        if (queue_->Get(&lpkt, true) < 0) {
          return -1;
        }
        if (lpkt.data == fls->data) {
          avcodec_flush_buffers(avctx_);
          SetFinished(false);
        }
      } while (lpkt.data == fls->data);
      av_packet_unref(&pkt_);
      pkt_temp = pkt_ = lpkt;
      packet_pending_ = true;
    }

    int ret = avcodec_decode_audio4(avctx_, frame, &got_frame, &pkt_temp);
    if (got_frame) {
      AVRational tb = {1, frame->sample_rate};
      if (IsValidPts(frame->pts)) {
        AVRational stb = av_codec_get_pkt_timebase(avctx_);
        frame->pts = av_rescale_q(frame->pts, stb, tb);
      } else {
        NOTREACHED();
      }
    }

    if (ret < 0) {
      packet_pending_ = false;
    } else {
      pkt_temp.dts = pkt_temp.pts = invalid_pts();
      if (pkt_temp.data) {
        pkt_temp.data += ret;
        pkt_temp.size -= ret;
        if (pkt_temp.size <= 0) {
          packet_pending_ = false;
        }
      } else {
        if (!got_frame) {
          packet_pending_ = false;
          SetFinished(true);
        }
      }
    }
  } while (!got_frame && !Finished());

  return got_frame;
}

VideoDecoder::VideoDecoder(AVCodecContext* avctx, PacketQueue* queue)
    : IFrameDecoder(avctx, queue) {
  CHECK(CodecType() == AVMEDIA_TYPE_VIDEO);
}

int VideoDecoder::width() const {
  return avctx_->width;
}

int VideoDecoder::height() const {
  return avctx_->height;
}

int VideoDecoder::DecodeFrame(AVFrame* frame) {
  int got_frame = 0;
  AVPacket pkt_temp;
  static const AVPacket* fls = PacketQueue::FlushPkt();
  do {
    if (queue_->IsAborted()) {
      return -1;
    }

    if (!packet_pending_) {
      AVPacket lpkt;
      do {
        if (queue_->Get(&lpkt, true) < 0) {
          return -1;
        }
        if (lpkt.data == fls->data) {
          avcodec_flush_buffers(avctx_);
          SetFinished(false);
        }
      } while (lpkt.data == fls->data);
      av_packet_unref(&pkt_);
      pkt_temp = pkt_ = lpkt;
      packet_pending_ = true;
    }

    int ret = avcodec_decode_video2(avctx_, frame, &got_frame, &pkt_temp);
    if (got_frame) {
      frame->pts = av_frame_get_best_effort_timestamp(frame);
    }

    if (ret < 0) {
      packet_pending_ = false;
    } else {
      pkt_temp.dts = pkt_temp.pts = invalid_pts();
      if (pkt_temp.data) {
        ret = pkt_temp.size;
        pkt_temp.data += ret;
        pkt_temp.size -= ret;
        if (pkt_temp.size <= 0) {
          packet_pending_ = false;
        }
      } else {
        if (!got_frame) {
          packet_pending_ = false;
          SetFinished(true);
        }
      }
    }
  } while (!got_frame && !Finished());

  return got_frame;
}

}  // namespace core
}
}
}
