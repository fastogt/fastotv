#pragma once

#include "core/packet_queue.h"

class Decoder {
 public:
  class DecoderClient {
   public:
    virtual void HandleEmptyQueue(Decoder* dec) = 0;
    virtual ~DecoderClient();
  };
  virtual ~Decoder();

  void Start();
  void Abort();
  int GetPktSerial() const;

  bool Finished() const;
  void SetFinished(bool finished);

  AVMediaType CodecType() const;

  int64_t start_pts;
  AVRational start_pts_tb;

 protected:
  Decoder(AVCodecContext* avctx, PacketQueue* queue, DecoderClient* client);

  AVCodecContext* avctx_;
  AVPacket pkt_;
  PacketQueue* const queue_;

  bool packet_pending_;
  int pkt_serial_;

  int64_t next_pts_;
  AVRational next_pts_tb_;
  DecoderClient* const client_;

 private:
  bool finished_;
};

class IFrameDecoder : public Decoder {
 public:
  IFrameDecoder(AVCodecContext* avctx, PacketQueue* queue, DecoderClient* client);
  virtual int DecodeFrame(AVFrame* frame) = 0;
};

class ISubDecoder : public Decoder {
 public:
  ISubDecoder(AVCodecContext* avctx, PacketQueue* queue, DecoderClient* client);
  virtual int DecodeFrame(AVSubtitle* sub) = 0;
};

class AudioDecoder : public IFrameDecoder {
 public:
  AudioDecoder(AVCodecContext* avctx, PacketQueue* queue, DecoderClient* client);
  virtual int DecodeFrame(AVFrame* frame) override;
};

class VideoDecoder : public IFrameDecoder {
 public:
  VideoDecoder(AVCodecContext* avctx,
               PacketQueue* queue,
               DecoderClient* client,
               int decoder_reorder_pts);

  int width() const;
  int height() const;

  int64_t PtsCorrectionNumFaultyDts() const;
  int64_t PtsCorrectionNumFaultyPts() const;

  int DecodeFrame(AVFrame* frame) override;

 private:
  int decoder_reorder_pts_;
};

class SubDecoder : public ISubDecoder {
 public:
  SubDecoder(AVCodecContext* avctx, PacketQueue* queue, DecoderClient* client);

  int width() const;
  int height() const;

  int DecodeFrame(AVSubtitle* sub) override;
};
