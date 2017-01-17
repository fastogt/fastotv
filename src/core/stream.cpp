#include "core/stream.h"

#define MIN_FRAMES 25

Stream::Stream() : stream_index_(-1), stream_st_(NULL) {}

Stream::Stream(int index, AVStream* av_stream_st)
    : stream_index_(index), stream_st_(av_stream_st) {}

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

int Stream::HasEnoughPackets(PacketQueue* queue) {
  return stream_index_ < 0 || queue->abort_request() ||
         (stream_st_->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
         (queue->nb_packets() > MIN_FRAMES &&
          (!queue->duration() || q2d() * queue->duration() > 1.0));
}

Stream::~Stream() {
  stream_index_ = -1;
  stream_st_ = NULL;
}

int Stream::Index() const {
  return stream_index_;
}

AVStream* Stream::AvStream() const {
  return stream_st_;
}

double Stream::q2d() const {
  return av_q2d(stream_st_->time_base);
}

VideoStream::VideoStream() : Stream() {}

VideoStream::VideoStream(int index, AVStream* av_stream_st) : Stream(index, av_stream_st) {}

VideoDecoder* VideoStream::CreateDecoder(AVCodecContext* avctx,
                                         PacketQueue* queue,
                                         SDL_cond* empty_queue_cond,
                                         int decoder_reorder_pts) const {
  return new VideoDecoder(avctx, queue, empty_queue_cond, decoder_reorder_pts);
}

AudioStream::AudioStream() : Stream() {}

AudioStream::AudioStream(int index, AVStream* av_stream_st) : Stream(index, av_stream_st) {}

AudioDecoder* AudioStream::CreateDecoder(AVCodecContext* avctx,
                                         PacketQueue* queue,
                                         SDL_cond* empty_queue_cond) const {
  return new AudioDecoder(avctx, queue, empty_queue_cond);
}

SubtitleStream::SubtitleStream() : Stream() {}

SubtitleStream::SubtitleStream(int index, AVStream* av_stream_st) : Stream(index, av_stream_st) {}

SubDecoder* SubtitleStream::CreateDecoder(AVCodecContext* avctx,
                                          PacketQueue* queue,
                                          SDL_cond* empty_queue_cond) const {
  return new SubDecoder(avctx, queue, empty_queue_cond);
}
