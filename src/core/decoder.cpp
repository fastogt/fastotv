#include "core/decoder.h"

namespace core {

Decoder::DecoderClient::~DecoderClient() {}

Decoder::Decoder(AVCodecContext* avctx, PacketQueue* queue, DecoderClient* client)
    : avctx_(avctx),
      pkt_(),
      queue_(queue),
      packet_pending_(false),
      pkt_serial_(0),
      client_(client),
      finished_(false) {}

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

int Decoder::GetPktSerial() const {
  return pkt_serial_;
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

IFrameDecoder::IFrameDecoder(AVCodecContext* avctx, PacketQueue* queue, DecoderClient* client)
    : Decoder(avctx, queue, client) {}

AudioDecoder::AudioDecoder(AVCodecContext* avctx, PacketQueue* queue, DecoderClient* client)
    : IFrameDecoder(avctx, queue, client),
      start_pts_(AV_NOPTS_VALUE),
      start_pts_tb_{0, 0},
      next_pts_(AV_NOPTS_VALUE),
      next_pts_tb_{0, 0} {
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
    if (queue_->AbortRequest()) {
      return -1;
    }

    if (!packet_pending_ || queue_->Serial() != pkt_serial_) {
      AVPacket lpkt;
      do {
        if (queue_->NbPackets() == 0) {
          if (client_) {
            client_->HandleEmptyQueue(this);
          }
        }
        if (queue_->Get(&lpkt, true, &pkt_serial_) < 0) {
          return -1;
        }
        if (lpkt.data == fls->data) {
          avcodec_flush_buffers(avctx_);
          SetFinished(false);
          next_pts_ = start_pts_;
          next_pts_tb_ = start_pts_tb_;
        }
      } while (lpkt.data == fls->data || queue_->Serial() != pkt_serial_);
      av_packet_unref(&pkt_);
      pkt_temp = pkt_ = lpkt;
      packet_pending_ = true;
    }

    int ret = avcodec_decode_audio4(avctx_, frame, &got_frame, &pkt_temp);
    if (got_frame) {
      AVRational tb = {1, frame->sample_rate};
      if (frame->pts != AV_NOPTS_VALUE) {
        frame->pts = av_rescale_q(frame->pts, av_codec_get_pkt_timebase(avctx_), tb);
      } else if (next_pts_ != AV_NOPTS_VALUE) {
        frame->pts = av_rescale_q(next_pts_, next_pts_tb_, tb);
      }
      if (frame->pts != AV_NOPTS_VALUE) {
        next_pts_ = frame->pts + frame->nb_samples;
        next_pts_tb_ = tb;
      }
    }

    if (ret < 0) {
      packet_pending_ = false;
    } else {
      pkt_temp.dts = pkt_temp.pts = AV_NOPTS_VALUE;
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

VideoDecoder::VideoDecoder(AVCodecContext* avctx,
                           PacketQueue* queue,
                           DecoderClient* client,
                           int decoder_reorder_pts)
    : IFrameDecoder(avctx, queue, client), decoder_reorder_pts_(decoder_reorder_pts) {
  CHECK(CodecType() == AVMEDIA_TYPE_VIDEO);
}

int VideoDecoder::width() const {
  return avctx_->width;
}

int VideoDecoder::height() const {
  return avctx_->height;
}

int64_t VideoDecoder::PtsCorrectionNumFaultyDts() const {
  return avctx_->pts_correction_num_faulty_dts;
}

int64_t VideoDecoder::PtsCorrectionNumFaultyPts() const {
  return avctx_->pts_correction_num_faulty_pts;
}

int VideoDecoder::DecodeFrame(AVFrame* frame) {
  int got_frame = 0;
  AVPacket pkt_temp;
  static const AVPacket* fls = PacketQueue::FlushPkt();
  do {
    if (queue_->AbortRequest()) {
      return -1;
    }

    if (!packet_pending_ || queue_->Serial() != pkt_serial_) {
      AVPacket lpkt;
      do {
        if (queue_->NbPackets() == 0) {
          if (client_) {
            client_->HandleEmptyQueue(this);
          }
        }
        if (queue_->Get(&lpkt, true, &pkt_serial_) < 0) {
          return -1;
        }
        if (lpkt.data == fls->data) {
          avcodec_flush_buffers(avctx_);
          SetFinished(false);
        }
      } while (lpkt.data == fls->data || queue_->Serial() != pkt_serial_);
      av_packet_unref(&pkt_);
      pkt_temp = pkt_ = lpkt;
      packet_pending_ = true;
    }

    int ret = avcodec_decode_video2(avctx_, frame, &got_frame, &pkt_temp);
    if (got_frame) {
      if (decoder_reorder_pts_ == -1) {
        frame->pts = av_frame_get_best_effort_timestamp(frame);
      } else if (!decoder_reorder_pts_) {
        frame->pts = frame->pkt_dts;
      }
    }

    if (ret < 0) {
      packet_pending_ = false;
    } else {
      pkt_temp.dts = pkt_temp.pts = AV_NOPTS_VALUE;
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
