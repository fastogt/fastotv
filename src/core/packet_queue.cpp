#include "core/packet_queue.h"

SAVPacket::SAVPacket(const AVPacket& p) : pkt(p) {}

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

int PacketQueue::Get(AVPacket* pkt, int block, int* serial) {
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

PacketQueue* PacketQueue::MakePacketQueue(int** ext_serial) {
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

bool PacketQueue::AbortRequest() const {
  return abort_request_;
}

size_t PacketQueue::NbPackets() const {
  return queue_.size();
}

int PacketQueue::Size() const {
  return size_;
}

int64_t PacketQueue::Duration() const {
  return duration_;
}

int PacketQueue::Serial() const {
  return serial_;
}

void PacketQueue::Start() {
  lock_t lock(mutex_);
  abort_request_ = false;
  PutPrivate(FlushPkt());
}

int PacketQueue::Put(AVPacket* pkt) {
  int ret;
  {
    lock_t lock(mutex_);
    ret = PutPrivate(pkt);
  }
  if (pkt != PacketQueue::FlushPkt() && ret < 0) {
    av_packet_unref(pkt);
  }

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

int PacketQueue::PutPrivate(AVPacket* pkt) {
  if (abort_request_) {
    return -1;
  }

  SAVPacket* pkt1 = new SAVPacket(*pkt);
  if (pkt == FlushPkt()) {
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
