#pragma once

#include <common/event.h>

namespace core {
struct VideoFrame;
}

class VideoState;

enum EventsType { ALLOC_FRAME_EVENT, QUIT_STREAM_EVENT };

class IBaseEvent : public common::IEvent<EventsType> {
 public:
  typedef common::IEvent<EventsType> base_class;
  typedef base_class::type_t type_t;
  typedef void senders_t;

  senders_t* sender() const;
  virtual ~IBaseEvent();

 protected:
  IBaseEvent(senders_t* sender, type_t event_t);

 private:
  senders_t* sender_;
};

class StreamEvent : public IBaseEvent {
 public:
  typedef IBaseEvent base_class;
  typedef base_class::type_t type_t;
  typedef base_class::senders_t senders_t;
  typedef VideoState stream_t;

  stream_t* Stream() const;

 protected:
  StreamEvent(senders_t* sender, type_t event_t, stream_t* stream);

 private:
  stream_t* stream_;
};

class AllocFrameEvent : public StreamEvent {
 public:
  typedef StreamEvent base_class;
  typedef base_class::type_t type_t;
  typedef base_class::senders_t senders_t;
  typedef base_class::stream_t stream_t;

  AllocFrameEvent(senders_t* sender, stream_t* stream, core::VideoFrame* frame);
  core::VideoFrame* Frame() const;

 private:
  core::VideoFrame* frame_;
};

class QuitStreamEvent : public StreamEvent {
 public:
  typedef StreamEvent base_class;
  typedef base_class::type_t type_t;
  typedef base_class::senders_t senders_t;
  typedef base_class::stream_t stream_t;

  QuitStreamEvent(senders_t* sender, stream_t* stream, int code);
  int Code() const;

 private:
  int code_;
};
