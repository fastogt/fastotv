#include "core/decoder.h"

Decoder::Decoder(AVCodecContext* avctx, PacketQueue* queue, SDL_cond* empty_queue_cond)
    : start_pts(AV_NOPTS_VALUE),
      start_pts_tb{0, 0},
      avctx_(avctx),
      pkt_(),
      queue_(queue),
      packet_pending_(false),
      finished_(false),
      empty_queue_cond_(empty_queue_cond),
      pkt_serial_(0),
      next_pts_(0),
      next_pts_tb_{0, 0},
      decoder_tid_(NULL) {}

int Decoder::Start(int (*fn)(void*), void* arg) {
  queue_->start();
  decoder_tid_ = SDL_CreateThread(fn, "decoder", arg);
  if (!decoder_tid_) {
    av_log(NULL, AV_LOG_ERROR, "SDL_CreateThread(): %s\n", SDL_GetError());
    return AVERROR(ENOMEM);
  }
  return 0;
}

void Decoder::Abort(FrameQueue* fq) {
  queue_->abort();
  fq->signal();
  SDL_WaitThread(decoder_tid_, NULL);
  decoder_tid_ = NULL;
  queue_->flush();
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

IFrameDecoder::IFrameDecoder(AVCodecContext* avctx, PacketQueue* queue, SDL_cond* empty_queue_cond)
    : Decoder(avctx, queue, empty_queue_cond) {}

ISubDecoder::ISubDecoder(AVCodecContext* avctx, PacketQueue* queue, SDL_cond* empty_queue_cond)
    : Decoder(avctx, queue, empty_queue_cond) {}

AudioDecoder::AudioDecoder(AVCodecContext* avctx, PacketQueue* queue, SDL_cond* empty_queue_cond)
    : IFrameDecoder(avctx, queue, empty_queue_cond) {
  CHECK(CodecType() == AVMEDIA_TYPE_AUDIO);
}

int AudioDecoder::DecodeFrame(AVFrame* frame) {
  int got_frame = 0;
  AVPacket pkt_temp;
  static const AVPacket* fls = PacketQueue::flush_pkt();
  do {
    int ret = -1;

    if (queue_->abort_request()) {
      return -1;
    }

    if (!packet_pending_ || queue_->serial() != pkt_serial_) {
      AVPacket lpkt;
      do {
        if (queue_->nb_packets() == 0) {
          SDL_CondSignal(empty_queue_cond_);
        }
        if (queue_->get(&lpkt, 1, &pkt_serial_) < 0) {
          return -1;
        }
        if (lpkt.data == fls->data) {
          avcodec_flush_buffers(avctx_);
          SetFinished(false);
          next_pts_ = start_pts;
          next_pts_tb_ = start_pts_tb;
        }
      } while (lpkt.data == fls->data || queue_->serial() != pkt_serial_);
      av_packet_unref(&pkt_);
      pkt_temp = pkt_ = lpkt;
      packet_pending_ = true;
    }

    ret = avcodec_decode_audio4(avctx_, frame, &got_frame, &pkt_temp);
    if (got_frame) {
      AVRational tb = (AVRational){1, frame->sample_rate};
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
          SetFinished(pkt_serial_);
        }
      }
    }
  } while (!got_frame && !Finished());

  return got_frame;
}

VideoDecoder::VideoDecoder(AVCodecContext* avctx,
                           PacketQueue* queue,
                           SDL_cond* empty_queue_cond,
                           int decoder_reorder_pts)
    : IFrameDecoder(avctx, queue, empty_queue_cond), decoder_reorder_pts_(decoder_reorder_pts) {
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
  static const AVPacket* fls = PacketQueue::flush_pkt();
  do {
    int ret = -1;

    if (queue_->abort_request()) {
      return -1;
    }

    if (!packet_pending_ || queue_->serial() != pkt_serial_) {
      AVPacket lpkt;
      do {
        if (queue_->nb_packets() == 0) {
          SDL_CondSignal(empty_queue_cond_);
        }
        if (queue_->get(&lpkt, 1, &pkt_serial_) < 0) {
          return -1;
        }
        if (lpkt.data == fls->data) {
          avcodec_flush_buffers(avctx_);
          SetFinished(false);
          next_pts_ = start_pts;
          next_pts_tb_ = start_pts_tb;
        }
      } while (lpkt.data == fls->data || queue_->serial() != pkt_serial_);
      av_packet_unref(&pkt_);
      pkt_temp = pkt_ = lpkt;
      packet_pending_ = true;
    }

    ret = avcodec_decode_video2(avctx_, frame, &got_frame, &pkt_temp);
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
          SetFinished(pkt_serial_);
        }
      }
    }
  } while (!got_frame && !Finished());

  return got_frame;
}

SubDecoder::SubDecoder(AVCodecContext* avctx, PacketQueue* queue, SDL_cond* empty_queue_cond)
    : ISubDecoder(avctx, queue, empty_queue_cond) {
  CHECK(CodecType() == AVMEDIA_TYPE_SUBTITLE);
}

int SubDecoder::width() const {
  return avctx_->width;
}

int SubDecoder::height() const {
  return avctx_->height;
}

int SubDecoder::DecodeFrame(AVSubtitle* sub) {
  int got_frame = 0;
  AVPacket pkt_temp;
  static const AVPacket* fls = PacketQueue::flush_pkt();
  do {
    int ret = -1;

    if (queue_->abort_request()) {
      return -1;
    }

    if (!packet_pending_ || queue_->serial() != pkt_serial_) {
      AVPacket lpkt;
      do {
        if (queue_->nb_packets() == 0) {
          SDL_CondSignal(empty_queue_cond_);
        }
        if (queue_->get(&lpkt, 1, &pkt_serial_) < 0) {
          return -1;
        }
        if (lpkt.data == fls->data) {
          avcodec_flush_buffers(avctx_);
          SetFinished(false);
          next_pts_ = start_pts;
          next_pts_tb_ = start_pts_tb;
        }
      } while (lpkt.data == fls->data || queue_->serial() != pkt_serial_);
      av_packet_unref(&pkt_);
      pkt_temp = pkt_ = lpkt;
      packet_pending_ = true;
    }

    ret = avcodec_decode_subtitle2(avctx_, sub, &got_frame, &pkt_temp);
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
          SetFinished(pkt_serial_);
        }
      }
    }
  } while (!got_frame && !Finished());

  return got_frame;
}
