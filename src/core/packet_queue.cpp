#include "core/packet_queue.h"

SAVPacket::SAVPacket(const AVPacket& p) : pkt(p) {}

PacketQueue::PacketQueue()
    : serial(0), list_(), size_(0), duration_(0), abort_request_(true), mutex(NULL), cond(NULL) {
  mutex = SDL_CreateMutex();
  if (!mutex) {
    av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
    return;
  }
  cond = SDL_CreateCond();
  if (!cond) {
    av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
    return;
  }
}

int PacketQueue::put_nullpacket(int stream_index) {
  AVPacket pkt1, *pkt = &pkt1;
  av_init_packet(pkt);
  pkt->data = NULL;
  pkt->size = 0;
  pkt->stream_index = stream_index;
  return put(pkt);
}

int PacketQueue::get(AVPacket* pkt, int block, int* serial) {
  int ret = 0;

  SDL_LockMutex(mutex);
  while (true) {
    if (abort_request_) {
      ret = -1;
      break;
    }

    if (!list_.empty()) {
      SAVPacket* pkt1 = list_[0];
      list_.pop_front();
      size_ -= pkt1->pkt.size + sizeof(*pkt1);
      duration_ -= pkt1->pkt.duration;
      *pkt = pkt1->pkt;
      if (serial) {
        *serial = pkt1->serial;
      }
      delete pkt1;
      ret = 1;
      break;
    } else if (!block) {
      ret = 0;
      break;
    } else {
      SDL_CondWait(cond, mutex);
    }
  }
  SDL_UnlockMutex(mutex);
  return ret;
}

AVPacket* PacketQueue::flush_pkt() {
  static AVPacket fls;
  av_init_packet(&fls);
  fls.data = reinterpret_cast<uint8_t*>(&fls);
  return &fls;
}

bool PacketQueue::abortRequest() const {
  return abort_request_;
}

size_t PacketQueue::nbPackets() const {
  return list_.size();
}

int PacketQueue::size() const {
  return size_;
}

int64_t PacketQueue::duration() const {
  return duration_;
}

void PacketQueue::start() {
  SDL_LockMutex(mutex);
  abort_request_ = false;
  put_private(flush_pkt());
  SDL_UnlockMutex(mutex);
}

int PacketQueue::put(AVPacket* pkt) {
  int ret;

  SDL_LockMutex(mutex);
  ret = put_private(pkt);
  SDL_UnlockMutex(mutex);

  if (pkt != PacketQueue::flush_pkt() && ret < 0) {
    av_packet_unref(pkt);
  }

  return ret;
}

void PacketQueue::flush() {
  SDL_LockMutex(mutex);
  for (auto it = list_.begin(); it != list_.end(); ++it) {
    SAVPacket* pkt = *it;
    av_packet_unref(&pkt->pkt);
    delete pkt;
  }
  list_.clear();
  size_ = 0;
  duration_ = 0;
  SDL_UnlockMutex(mutex);
}

void PacketQueue::abort() {
  SDL_LockMutex(mutex);
  abort_request_ = true;
  SDL_CondSignal(cond);
  SDL_UnlockMutex(mutex);
}

PacketQueue::~PacketQueue() {
  flush();
  SDL_DestroyMutex(mutex);
  SDL_DestroyCond(cond);
}

int PacketQueue::put_private(AVPacket* pkt) {
  if (abort_request_) {
    return -1;
  }

  SAVPacket* pkt1 = new SAVPacket(*pkt);
  if (pkt == flush_pkt()) {
    serial++;
  }
  pkt1->serial = serial;

  list_.push_back(pkt1);
  size_ += pkt1->pkt.size + sizeof(*pkt1);
  duration_ += pkt1->pkt.duration;
  /* XXX: should duplicate packet data in DV case */
  SDL_CondSignal(cond);
  return 0;
}
