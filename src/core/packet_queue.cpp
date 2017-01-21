#include "core/packet_queue.h"

SAVPacket::SAVPacket(const AVPacket& p) : pkt(p) {}

PacketQueue::PacketQueue()
    : serial_(0), queue_(), size_(0), duration_(0), abort_request_(true), cond_(), mutex_() {}

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

  lock_t lock(mutex_);
  while (true) {
    if (abort_request_) {
      ret = -1;
      break;
    }

    if (!queue_.empty()) {
      SAVPacket* pkt1 = queue_[0];
      queue_.pop_front();
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
      cond_.wait(lock);
    }
  }
  return ret;
}

PacketQueue* PacketQueue::make_packet_queue(int** ext_serial) {
  PacketQueue* pq = new PacketQueue;
  if (ext_serial) {
    *ext_serial = &pq->serial_;
  }
  return pq;
}

AVPacket* PacketQueue::flush_pkt() {
  static AVPacket fls;
  av_init_packet(&fls);
  fls.data = reinterpret_cast<uint8_t*>(&fls);
  return &fls;
}

bool PacketQueue::abort_request() const {
  return abort_request_;
}

size_t PacketQueue::nb_packets() const {
  return queue_.size();
}

int PacketQueue::size() const {
  return size_;
}

int64_t PacketQueue::duration() const {
  return duration_;
}

int PacketQueue::serial() const {
  return serial_;
}

void PacketQueue::start() {
  lock_t lock(mutex_);
  abort_request_ = false;
  put_private(flush_pkt());
}

int PacketQueue::put(AVPacket* pkt) {
  int ret;
  {
    lock_t lock(mutex_);
    ret = put_private(pkt);
  }
  if (pkt != PacketQueue::flush_pkt() && ret < 0) {
    av_packet_unref(pkt);
  }

  return ret;
}

void PacketQueue::flush() {
  lock_t lock(mutex_);
  for (auto it = queue_.begin(); it != queue_.end(); ++it) {
    SAVPacket* pkt = *it;
    av_packet_unref(&pkt->pkt);
    delete pkt;
  }
  queue_.clear();
  size_ = 0;
  duration_ = 0;
}

void PacketQueue::abort() {
  lock_t lock(mutex_);
  abort_request_ = true;
  cond_.notify_one();
}

PacketQueue::~PacketQueue() {
  flush();
}

int PacketQueue::put_private(AVPacket* pkt) {
  if (abort_request_) {
    return -1;
  }

  SAVPacket* pkt1 = new SAVPacket(*pkt);
  if (pkt == flush_pkt()) {
    serial_++;
  }
  pkt1->serial = serial_;

  queue_.push_back(pkt1);
  size_ += pkt1->pkt.size + sizeof(*pkt1);
  duration_ += pkt1->pkt.duration;
  /* XXX: should duplicate packet data in DV case */
  cond_.notify_one();
  return 0;
}
