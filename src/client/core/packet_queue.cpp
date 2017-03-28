/*  Copyright (C) 2014-2017 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

#include "client/core/packet_queue.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

SAVPacket::SAVPacket(const AVPacket& p) : pkt(p) {}

PacketQueue::PacketQueue()
    : queue_(), size_(0), duration_(0), abort_request_(true), cond_(), mutex_() {}

int PacketQueue::PutNullpacket(int stream_index) {
  AVPacket pkt1, *pkt = &pkt1;
  av_init_packet(pkt);
  pkt->data = NULL;
  pkt->size = 0;
  pkt->stream_index = stream_index;
  return Put(pkt);
}

int PacketQueue::Get(AVPacket* pkt) {
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
      delete pkt1;
      ret = 1;
      break;
    } else {
      cond_.wait(lock);
    }
  }
  return ret;
}

bool PacketQueue::IsAborted() {
  lock_t lock(mutex_);
  return abort_request_;
}

size_t PacketQueue::NbPackets() {
  lock_t lock(mutex_);
  return queue_.size();
}

int PacketQueue::Size() const {
  return size_;
}

int64_t PacketQueue::Duration() const {
  return duration_;
}

void PacketQueue::Start() {
  lock_t lock(mutex_);
  abort_request_ = false;
  cond_.notify_one();
}

int PacketQueue::Put(AVPacket* pkt) {
  SAVPacket* sav = new SAVPacket(*pkt);
  lock_t lock(mutex_);
  if (abort_request_) {
    av_packet_unref(pkt);
    delete sav;
    return -1;
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

int PacketQueue::PutPrivate(SAVPacket* pkt1) {
  queue_.push_back(pkt1);
  size_ += pkt1->pkt.size + sizeof(*pkt1);
  duration_ += pkt1->pkt.duration;
  /* XXX: should duplicate packet data in DV case */
  return 0;
}

}  // namespace core
}
}
}
