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

#include "server/redis/redis_pub_sub.h"

#include <stddef.h>  // for NULL

#include <hiredis/hiredis.h>  // for redisFree, freeReplyObject, redisCommand

#include <common/logger.h>  // for COMPACT_LOG_WARNING, WARNING_LOG
#include <common/utils.h>

#include "server/redis/redis_connect.h"

namespace fasto {
namespace fastotv {
namespace server {

RedisPubSub::RedisPubSub(RedisSubHandler* handler) : handler_(handler), stop_(false) {}

void RedisPubSub::SetConfig(const RedisSubConfig& config) {
  config_ = config;
}

void RedisPubSub::Listen() {
  redisContext* redis_sub = redis_connect(config_);
  if (!redis_sub) {
    return;
  }

  const char* channel_str = config_.channel_in.c_str();

  void* reply = redisCommand(redis_sub, "SUBSCRIBE %s", channel_str);
  if (!reply) {
    redisFree(redis_sub);
    return;
  }

  while (!stop_) {
    redisReply* lreply = NULL;
    void** plreply = reinterpret_cast<void**>(&lreply);
    if (redisGetReply(redis_sub, plreply) != REDIS_OK) {
      WARNING_LOG() << "REDIS PUB/SUB GET REPLY ERROR: " << redis_sub->errstr;
      break;
    }

    if (!lreply) {
      break;
    }

    bool is_error_reply = lreply->type != REDIS_REPLY_ARRAY || lreply->elements != 3 ||
                          lreply->element[1]->type != REDIS_REPLY_STRING ||
                          lreply->element[2]->type != REDIS_REPLY_STRING;
    UNUSED(is_error_reply);

    char* chn = lreply->element[1]->str;
    size_t chn_len = lreply->element[1]->len;
    char* msg = lreply->element[2]->str;
    size_t msg_len = lreply->element[2]->len;

    if (handler_) {
      handler_->HandleMessage(std::string(chn, chn_len), std::string(msg, msg_len));
    }

    freeReplyObject(lreply);
  }

  freeReplyObject(reply);
  redisFree(redis_sub);
}

void RedisPubSub::Stop() {
  stop_ = true;
}

bool RedisPubSub::PublishStateToChannel(const std::string& msg) {
  return Publish(config_.channel_clients_state, msg);
}

bool RedisPubSub::PublishToChannelOut(const std::string& msg) {
  return Publish(config_.channel_out, msg);
}

bool RedisPubSub::Publish(const std::string& channel, const std::string& msg) {
  if (channel.empty()) {
    return false;
  }

  if (msg.empty()) {
    return false;
  }

  redisContext* redis_sub = redis_connect(config_);
  if (!redis_sub) {
    return false;
  }

  const char* chn = common::utils::c_strornull(channel);
  const char* m = common::utils::c_strornull(msg);
  void* rreply = redisCommand(redis_sub, "PUBLISH %s %s", chn, m);
  if (!rreply) {
    redisFree(redis_sub);
    return false;
  }

  freeReplyObject(rreply);
  redisFree(redis_sub);
  return true;
}

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
