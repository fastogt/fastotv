#pragma once

#include "ffmpeg_config.h"
extern "C" {
#include <libavformat/avformat.h>
}

#include <common/macros.h>

#include "core/decoder.h"

class Stream {
 public:
  int HasEnoughPackets(PacketQueue* queue);
  virtual bool Open(int index, AVStream* av_stream_st);
  bool IsOpened() const;
  virtual void Close();
  virtual ~Stream();

  int Index() const;
  AVStream* AvStream() const;
  double q2d() const;

 protected:
  Stream();
  Stream(int index, AVStream* av_stream_st);

 private:
  int stream_index_;
  AVStream* stream_st_;
};

class VideoStream : public Stream {
 public:
  VideoStream();
  VideoStream(int index, AVStream* av_stream_st);
  VideoDecoder* CreateDecoder(AVCodecContext* avctx,
                              PacketQueue* queue,
                              SDL_cond* empty_queue_cond,
                              int decoder_reorder_pts) const;
};

class AudioStream : public Stream {
 public:
  AudioStream();
  AudioStream(int index, AVStream* av_stream_st);
  AudioDecoder* CreateDecoder(AVCodecContext* avctx,
                              PacketQueue* queue,
                              SDL_cond* empty_queue_cond) const;
};

class SubtitleStream : public Stream {
 public:
  SubtitleStream();
  SubtitleStream(int index, AVStream* av_stream_st);
  SubDecoder* CreateDecoder(AVCodecContext* avctx,
                            PacketQueue* queue,
                            SDL_cond* empty_queue_cond) const;
};
