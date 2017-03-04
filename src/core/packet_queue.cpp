#include "core/packet_queue.h"

namespace core {

SAVPacket::SAVPacket(const AVPacket& p, serial_id_t serial) : pkt(p), serial(serial) {}

PacketQueue::PacketQueue()
    : serial_(0), queue_(), size_(0), duration_(0), abort_request_(true), cond_(), mutex_() {}

int PacketQueue::PutNullpacket(int stream_index) {
  AVPacket pkt1, *pkt = &pkt1;
  av_init_packet(pkt);
  pkt->data = NULL;
  pkt->size = 0;
  pkt->stream_index = stream_index;
  return Put(pkt);
}

int PacketQueue::Get(AVPacket* pkt, bool block, serial_id_t* serial) {
  if (!pkt) {
    return -1;
  }

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

PacketQueue* PacketQueue::MakePacketQueue(common::atomic<serial_id_t>** ext_serial) {
  PacketQueue* pq = new PacketQueue;
  if (ext_serial) {
    *ext_serial = &pq->serial_;
  }
  return pq;
}

AVPacket* PacketQueue::FlushPkt() {
  static AVPacket fls;
  av_init_packet(&fls);
  fls.data = reinterpret_cast<uint8_t*>(&fls);
  return &fls;
}

bool PacketQueue::AbortRequest() {
  lock_t lock(mutex_);
  return abort_request_;
}

size_t PacketQueue::NbPackets() {
  lock_t lock(mutex_);
  return queue_.size();
}

int PacketQueue::Size() {
  return size_;
}

int64_t PacketQueue::Duration() const {
  return duration_;
}

serial_id_t PacketQueue::Serial() const {
  return serial_;
}

void PacketQueue::Start() {
  SAVPacket* sav = new SAVPacket(*FlushPkt(), ++serial_);

  lock_t lock(mutex_);
  abort_request_ = false;
  PutPrivate(sav);
  cond_.notify_one();
}

int PacketQueue::Put(AVPacket* pkt) {
  bool is_flush = false;
  SAVPacket* sav = MakePacket(pkt, &is_flush);
  lock_t lock(mutex_);
  if (abort_request_) {
    if (!is_flush) {
      av_packet_unref(pkt);
      delete sav;
      return -1;
    }
  }

  int ret = PutPrivate(sav);
  cond_.notify_one();
  return ret;
}

void PacketQueue::Flush() {
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

void PacketQueue::Abort() {
  lock_t lock(mutex_);
  abort_request_ = true;
  cond_.notify_one();
}

PacketQueue::~PacketQueue() {
  Flush();
}

SAVPacket* PacketQueue::MakePacket(AVPacket* pkt, bool* is_flush) {
  if (pkt == FlushPkt()) {
    serial_++;
    *is_flush = true;
  }
  SAVPacket* pkt1 = new SAVPacket(*pkt, serial_);
  *is_flush = false;
  return pkt1;
}

int PacketQueue::PutPrivate(SAVPacket* pkt1) {
  queue_.push_back(pkt1);
  size_ += pkt1->pkt.size + sizeof(*pkt1);
  duration_ += pkt1->pkt.duration;
  /* XXX: should duplicate packet data in DV case */
  return 0;
}

}  // namespace core
