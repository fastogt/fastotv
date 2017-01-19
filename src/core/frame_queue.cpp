#include "core/frame_queue.h"

namespace {
void free_picture(VideoFrame* vp) {
  if (vp->bmp) {
    SDL_DestroyTexture(vp->bmp);
    vp->bmp = NULL;
  }
}

void frame_queue_unref_item(VideoFrame* vp) {
  av_frame_unref(vp->frame);
}
}

VideoFrameQueue::VideoFrameQueue(size_t max_size, bool keep_last)
    : mutex(NULL),
      cond(NULL),
      queue(),
      rindex_(0),
      rindex_shown_(0),
      windex_(0),
      size_(0),
      max_size_(FFMIN(max_size, VIDEO_PICTURE_QUEUE_SIZE)),
      keep_last_(keep_last),
      stoped_(false) {
  if (!(mutex = SDL_CreateMutex())) {
    av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
    return;
  }
  if (!(cond = SDL_CreateCond())) {
    av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
    return;
  }
  for (size_t i = 0; i < max_size; i++) {
    if (!(queue[i].frame = av_frame_alloc())) {
      return;
    }
  }
}

VideoFrameQueue::~VideoFrameQueue() {
  for (size_t i = 0; i < max_size_; i++) {
    VideoFrame* vp = &queue[i];
    frame_queue_unref_item(vp);
    av_frame_free(&vp->frame);
    free_picture(vp);
  }
  SDL_DestroyMutex(mutex);
  SDL_DestroyCond(cond);
}

void VideoFrameQueue::push() {
  if (++windex_ == max_size_) {
    windex_ = 0;
  }
  SDL_LockMutex(mutex);
  size_++;
  SDL_CondSignal(cond);
  SDL_UnlockMutex(mutex);
}

VideoFrame* VideoFrameQueue::peek_writable() {
  /* wait until we have space to put a new frame */
  SDL_LockMutex(mutex);
  while (size_ >= max_size_ && !stoped_) {
    SDL_CondWait(cond, mutex);
  }
  SDL_UnlockMutex(mutex);

  if (stoped_) {
    return nullptr;
  }

  return &queue[windex_];
}

/* return the number of undisplayed frames in the queue */
int VideoFrameQueue::nb_remaining() {
  return size_ - rindex_shown_;
}

VideoFrame* VideoFrameQueue::peek_last() {
  return &queue[rindex_];
}

VideoFrame* VideoFrameQueue::peek() {
  return &queue[(rindex_ + rindex_shown_) % max_size_];
}

VideoFrame* VideoFrameQueue::peek_next() {
  return &queue[(rindex_ + rindex_shown_ + 1) % max_size_];
}

VideoFrame* VideoFrameQueue::peek_readable() {
  /* wait until we have a readable a new frame */
  SDL_LockMutex(mutex);
  while (size_ - rindex_shown_ <= 0 && !stoped_) {
    SDL_CondWait(cond, mutex);
  }
  SDL_UnlockMutex(mutex);

  if (stoped_) {
    return nullptr;
  }

  return &queue[(rindex_ + rindex_shown_) % max_size_];
}

void VideoFrameQueue::Stop() {
  SDL_LockMutex(mutex);
  stoped_ = true;
  SDL_CondSignal(cond);
  SDL_UnlockMutex(mutex);
}

void VideoFrameQueue::next() {
  if (keep_last_ && !rindex_shown_) {
    rindex_shown_ = 1;
    return;
  }
  frame_queue_unref_item(&queue[rindex_]);
  if (++rindex_ == max_size_) {
    rindex_ = 0;
  }
  SDL_LockMutex(mutex);
  size_--;
  SDL_CondSignal(cond);
  SDL_UnlockMutex(mutex);
}

/* return last shown position */
bool VideoFrameQueue::last_pos(int64_t* pos, int serial) {
  if (!pos) {
    return false;
  }

  VideoFrame* fp = &queue[rindex_];
  if (rindex_shown_ && fp->serial == serial) {
    *pos = fp->pos;
    return true;
  } else {
    return -1;
  }
}

size_t VideoFrameQueue::rindex_shown() const {
  return rindex_shown_;
}

size_t VideoFrameQueue::windex() const {
  return windex_;
}

VideoFrame* VideoFrameQueue::windex_frame() {
  return &queue[windex_];
}
