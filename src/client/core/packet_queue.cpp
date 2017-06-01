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

PacketQueue::PacketQueue()
    : queue_(), size_(0), duration_(0), abort_request_(true), cond_(), mutex_() {}

int PacketQueue::PutNullpacket(int stream_index) {
  AVPacket pkt1, *pkt = &pkt1;
  av_init_packet(pkt);
  pkt->data = NULL;
  pkt->size = 0;
  pkt->stream_index = stream_index;
  return PushFront(pkt);
}

bool PacketQueue::Get(AVPacket* pkt) {
  if (!pkt) {
    return false;
  }

  lock_t lock(mutex_);
  while (queue_.empty() && !abort_request_) {
    cond_.wait(lock);
  }
  if (abort_request_) {
    return false;
  }

  *pkt = queue_[0];
  queue_.pop_front();
  size_ -= pkt->size;
  duration_ -= pkt->duration;
  return true;
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
  return PushBack(pkt);
}

int PacketQueue::PushFront(AVPacket* pkt) {
  lock_t lock(mutex_);
  if (abort_request_) {
    av_packet_unref(pkt);
    return -1;
  }

  queue_.push_front(*pkt);
  size_ += pkt->size;
  duration_ += pkt->duration;
  cond_.notify_one();
  return 0;
}

int PacketQueue::PushBack(AVPacket* pkt) {
  lock_t lock(mutex_);
  if (abort_request_) {
    av_packet_unref(pkt);
    return -1;
  }

  queue_.push_back(*pkt);
  size_ += pkt->size;
  duration_ += pkt->duration;
  cond_.notify_one();
  return 0;
}

void PacketQueue::Flush() {
  lock_t lock(mutex_);
  for (auto it = queue_.begin(); it != queue_.end(); ++it) {
    AVPacket pkt = *it;
    av_packet_unref(&pkt);
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

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
