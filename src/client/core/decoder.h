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

#include <stdint.h>  // for int64_t

extern "C" {
#include <libavcodec/avcodec.h>  // for AVCodecContext, AVPacket
#include <libavutil/avutil.h>    // for AVMediaType
#include <libavutil/frame.h>     // for AVFrame
#include <libavutil/rational.h>  // for AVRational
}

#include "client/core/types.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
class PacketQueue;
class Decoder {
 public:
  virtual ~Decoder();

  void Start();
  void Abort();

  bool Finished() const;
  void SetFinished(bool finished);

  AVMediaType CodecType() const;

 protected:
  Decoder(AVCodecContext* avctx, PacketQueue* queue);

  AVCodecContext* avctx_;
  PacketQueue* const queue_;

 private:
  bool finished_;
};

class IFrameDecoder : public Decoder {
 public:
  IFrameDecoder(AVCodecContext* avctx, PacketQueue* queue);
  virtual int DecodeFrame(AVFrame* frame) = 0;
};

class AudioDecoder : public IFrameDecoder {
 public:
  AudioDecoder(AVCodecContext* avctx, PacketQueue* queue);
  virtual int DecodeFrame(AVFrame* frame) override;

  void SetStartPts(int64_t start_pts, AVRational start_pts_tb);

 private:
  pts_t start_pts_;
  AVRational start_pts_tb_;
};

class VideoDecoder : public IFrameDecoder {
 public:
  VideoDecoder(AVCodecContext* avctx, PacketQueue* queue);

  int width() const;
  int height() const;

  int DecodeFrame(AVFrame* frame) override;
};
}
}
}
}
