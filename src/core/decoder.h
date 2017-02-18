#pragma once

#include <stdint.h>  // for int64_t

extern "C" {
#include <libavcodec/avcodec.h>  // for AVCodecContext, AVPacket
#include <libavutil/avutil.h>    // for AVMediaType
#include <libavutil/frame.h>     // for AVFrame
#include <libavutil/rational.h>  // for AVRational
}

namespace core {
class PacketQueue;
}

namespace core {

class Decoder {
 public:
  virtual ~Decoder();

  void Start();
  void Abort();
  int GetPktSerial() const;

  bool Finished() const;
  void SetFinished(bool finished);

  AVMediaType CodecType() const;

 protected:
  Decoder(AVCodecContext* avctx, PacketQueue* queue);

  AVCodecContext* avctx_;
  AVPacket pkt_;
  PacketQueue* const queue_;

  bool packet_pending_;
  int pkt_serial_;

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
  int64_t start_pts_;
  AVRational start_pts_tb_;
  int64_t next_pts_;
  AVRational next_pts_tb_;
};

class VideoDecoder : public IFrameDecoder {
 public:
  VideoDecoder(AVCodecContext* avctx, PacketQueue* queue, int decoder_reorder_pts);

  int width() const;
  int height() const;

  int64_t PtsCorrectionNumFaultyDts() const;
  int64_t PtsCorrectionNumFaultyPts() const;

  int DecodeFrame(AVFrame* frame) override;

 private:
  int decoder_reorder_pts_;
};
}
