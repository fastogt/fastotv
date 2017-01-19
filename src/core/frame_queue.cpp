#include "core/frame_queue.h"

namespace {
void free_picture(Frame* vp) {
  if (vp->bmp) {
    SDL_DestroyTexture(vp->bmp);
    vp->bmp = NULL;
  }
}

void frame_queue_unref_item(Frame* vp) {
  av_frame_unref(vp->frame);
  avsubtitle_free(&vp->sub);
}
}

FrameQueue::FrameQueue(PacketQueue* pktq, size_t max_size, bool keep_last)
    : mutex(NULL),
      cond(NULL),
      queue(),
      rindex_(0),
      rindex_shown_(0),
      windex_(0),
      size_(0),
      max_size_(FFMIN(max_size, FRAME_QUEUE_SIZE)),
      keep_last_(keep_last),
      pktq_(pktq) {
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

FrameQueue::~FrameQueue() {
  for (size_t i = 0; i < max_size_; i++) {
    Frame* vp = &queue[i];
    frame_queue_unref_item(vp);
    av_frame_free(&vp->frame);
    free_picture(vp);
  }
  SDL_DestroyMutex(mutex);
  SDL_DestroyCond(cond);
}

void FrameQueue::push() {
  if (++windex_ == max_size_) {
    windex_ = 0;
  }
  SDL_LockMutex(mutex);
  size_++;
  SDL_CondSignal(cond);
  SDL_UnlockMutex(mutex);
}

Frame* FrameQueue::peek_writable() {
  /* wait until we have space to put a new frame */
  SDL_LockMutex(mutex);
  while (size_ >= max_size_ && !pktq_->abort_request()) {
    SDL_CondWait(cond, mutex);
  }
  SDL_UnlockMutex(mutex);

  if (pktq_->abort_request()) {
    return nullptr;
  }

  return &queue[windex_];
}

/* return the number of undisplayed frames in the queue */
int FrameQueue::nb_remaining() {
  return size_ - rindex_shown_;
}

Frame* FrameQueue::peek_last() {
  return &queue[rindex_];
}

Frame* FrameQueue::peek() {
  return &queue[(rindex_ + rindex_shown_) % max_size_];
}

Frame* FrameQueue::peek_next() {
  return &queue[(rindex_ + rindex_shown_ + 1) % max_size_];
}

Frame* FrameQueue::peek_readable() {
  /* wait until we have a readable a new frame */
  SDL_LockMutex(mutex);
  while (size_ - rindex_shown_ <= 0 && !pktq_->abort_request()) {
    SDL_CondWait(cond, mutex);
  }
  SDL_UnlockMutex(mutex);

  if (pktq_->abort_request()) {
    return nullptr;
  }

  return &queue[(rindex_ + rindex_shown_) % max_size_];
}

void FrameQueue::signal() {
  SDL_LockMutex(mutex);
  SDL_CondSignal(cond);
  SDL_UnlockMutex(mutex);
}

void FrameQueue::next() {
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
int64_t FrameQueue::last_pos() {
  Frame* fp = &queue[rindex_];
  if (rindex_shown_ && fp->serial == pktq_->serial()) {
    return fp->pos;
  } else {
    return -1;
  }
}

size_t FrameQueue::rindex_shown() const {
  return rindex_shown_;
}

size_t FrameQueue::windex() const {
  return windex_;
}

Frame* FrameQueue::windex_frame() {
  return &queue[windex_];
}

FrameQueueEx::FrameQueueEx(size_t max_size, bool keep_last)
    : queue_cond_(),
      queue_mutex_(),
      queue_(),
      rindex_(0),
      rindex_shown_(0),
      windex_(0),
      size_(0),
      max_size_(FFMIN(max_size, FRAME_QUEUE_SIZE)),
      keep_last_(keep_last),
      stoped_(false) {
  for (size_t i = 0; i < max_size; i++) {
    if (!(queue_[i].frame = av_frame_alloc())) {
      return;
    }
  }
}

FrameQueueEx::~FrameQueueEx() {
  for (size_t i = 0; i < max_size_; i++) {
    Frame* vp = &queue_[i];
    frame_queue_unref_item(vp);
    av_frame_free(&vp->frame);
    free_picture(vp);
  }
}

bool FrameQueueEx::GetLastUsedPos(int64_t* pos, int serial) {
  if (!pos) {
    return false;
  }

  lock_t lock(queue_mutex_);
  Frame* fp = &queue_[rindex_];
  if (rindex_shown_ && fp->serial == serial) {
    *pos = fp->pos;
    return true;
  }
  return false;
}

Frame* FrameQueueEx::GetPeekReadable() {
  /* wait until we have a readable a new frame */
  lock_t lock(queue_mutex_);
  while (IsEmptyInner() && !stoped_) {
    queue_cond_.wait(lock);
  }

  if (stoped_) {
    return nullptr;
  }

  return &queue_[(rindex_ + rindex_shown_) % max_size_];
}

Frame* FrameQueueEx::GetPeekWritable() {
  /* wait until we have space to put a new frame */
  lock_t lock(queue_mutex_);
  while (IsFullInner() && !stoped_) {
    queue_cond_.wait(lock);
  }

  if (stoped_) {
    return nullptr;
  }

  return &queue_[windex_];
}

void FrameQueueEx::Push() {
  lock_t lock(queue_mutex_);
  if (++windex_ == max_size_) {
    windex_ = 0;
  }
  size_++;
  queue_cond_.notify_one();
}

void FrameQueueEx::Stop() {
  lock_t lock(queue_mutex_);
  stoped_ = true;
  queue_cond_.notify_one();
}

Frame* FrameQueueEx::PeekOrNull() {
  lock_t lock(queue_mutex_);
  if (IsEmptyInner()) {  // if is empty
    return nullptr;
  }
  return &queue_[(rindex_ + rindex_shown_) % max_size_];
}

void FrameQueueEx::MoveToNext() {
  lock_t lock(queue_mutex_);
  if (keep_last_ && !rindex_shown_) {
    rindex_shown_ = 1;
    return;
  }
  frame_queue_unref_item(&queue_[rindex_]);
  if (++rindex_ == max_size_) {
    rindex_ = 0;
  }
  size_--;
  queue_cond_.notify_one();
}

bool FrameQueueEx::IsEmpty() {
  lock_t lock(queue_mutex_);
  return IsEmptyInner();
}

int FrameQueueEx::NbRemainingInner() const {
  int res = size_ - rindex_shown_;
  DCHECK(res != -1);
  return res;
}

bool FrameQueueEx::IsFullInner() const {
  return size_ >= max_size_;
}

bool FrameQueueEx::IsEmptyInner() const {
  return NbRemainingInner() <= 0;
}
