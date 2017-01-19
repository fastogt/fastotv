#include "core/frame_queue.h"

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
    queue[i] = new VideoFrame;
  }
}

VideoFrameQueue::~VideoFrameQueue() {
  for (size_t i = 0; i < max_size_; i++) {
    VideoFrame* vp = queue[i];
    delete vp;
  }
  SDL_DestroyMutex(mutex);
  SDL_DestroyCond(cond);
}

void VideoFrameQueue::Push() {
  if (++windex_ == max_size_) {
    windex_ = 0;
  }
  SDL_LockMutex(mutex);
  size_++;
  SDL_CondSignal(cond);
  SDL_UnlockMutex(mutex);
}

VideoFrame* VideoFrameQueue::GetPeekWritable() {
  /* wait until we have space to put a new frame */
  SDL_LockMutex(mutex);
  while (size_ >= max_size_ && !stoped_) {
    SDL_CondWait(cond, mutex);
  }
  SDL_UnlockMutex(mutex);

  if (stoped_) {
    return nullptr;
  }
  return queue[windex_];
}

/* return the number of undisplayed frames in the queue */
int VideoFrameQueue::NbRemaining() {
  return size_ - rindex_shown_;
}

VideoFrame* VideoFrameQueue::PeekLast() {
  return queue[rindex_];
}

VideoFrame* VideoFrameQueue::Peek() {
  return queue[(rindex_ + rindex_shown_) % max_size_];
}

VideoFrame* VideoFrameQueue::PeekNext() {
  return queue[(rindex_ + rindex_shown_ + 1) % max_size_];
}

void VideoFrameQueue::Stop() {
  SDL_LockMutex(mutex);
  stoped_ = true;
  SDL_CondSignal(cond);
  SDL_UnlockMutex(mutex);
}

void VideoFrameQueue::MoveToNext() {
  if (keep_last_ && !rindex_shown_) {
    rindex_shown_ = 1;
    return;
  }
  queue[rindex_]->ClearFrame();
  if (++rindex_ == max_size_) {
    rindex_ = 0;
  }
  SDL_LockMutex(mutex);
  size_--;
  SDL_CondSignal(cond);
  SDL_UnlockMutex(mutex);
}

/* return last shown position */
bool VideoFrameQueue::GetLastUsedPos(int64_t* pos, int serial) {
  if (!pos) {
    return false;
  }

  VideoFrame* fp = queue[rindex_];
  if (rindex_shown_ && fp->serial == serial) {
    *pos = fp->pos;
    return true;
  }

  return false;
}

size_t VideoFrameQueue::RindexShown() const {
  return rindex_shown_;
}

VideoFrame* VideoFrameQueue::Windex() {
  return queue[windex_];
}

bool VideoFrameQueue::IsEmpty() {
  return NbRemaining() == 0;
}
