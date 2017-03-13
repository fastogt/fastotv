/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of SiteOnYourDevice.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "redis/redis_helpers.h"

#include <hiredis/hiredis.h>

#include <string>
#include <vector>

#include <common/utils.h>
#include <common/logger.h>

#include "third-party/json-c/json-c/json.h"

#define GET_ALL_USERS_REDIS_REQUEST "HGETALL tvusers"
#define GET_USER_1S "HGET tvusers %s"

#undef ERROR

namespace fasto {
namespace fastotv {
namespace server {
namespace {

redisContext* redis_connect(const redis_configuration_t& config) {
  const common::net::HostAndPort redisHost = config.redis_host;
  const std::string unixPath = config.redis_unix_socket;

  if (!redisHost.isValid() && unixPath.empty()) {
    return NULL;
  }

  struct redisContext* redis = NULL;
  if (unixPath.empty()) {
    redis = redisConnect(redisHost.host.c_str(), redisHost.port);
  } else {
    redis = redisConnectUnix(unixPath.c_str());
    if (!redis || redis->err) {
      if (redis) {
        ERROR_LOG() << "REDIS UNIX CONNECTION ERROR: " << redis->errstr;
        redisFree(redis);
        redis = NULL;
      }
      redis = redisConnect(redisHost.host.c_str(), redisHost.port);
    }
  }

  if (redis->err) {
    ERROR_LOG() << "REDIS CONNECTION ERROR: " << redis->errstr;
    redisFree(redis);
    return NULL;
  }

  return redis;
}

bool parse_user_json(const std::string& login, const char* userJson, UserInfo* out_info) {
  if (!userJson || !out_info) {
    return false;
  }

  json_object* obj = json_tokener_parse(userJson);
  if (!obj) {
    return false;
  }

  *out_info = UserInfo::MakeClass(obj);
  json_object_put(obj);
  return true;
}

}  // namespace

RedisStorage::RedisStorage() : config_() {}

void RedisStorage::setConfig(const redis_configuration_t& config) {
  config_ = config;
}

bool RedisStorage::findUserAuth(const AuthInfo& user) const {
  UserInfo uinf;
  return findUser(user, &uinf);
}

bool RedisStorage::findUser(const AuthInfo& user, UserInfo* uinf) const {
  if (!user.IsValid() || !uinf) {
    return false;
  }

  redisContext* redis = redis_connect(config_);
  if (!redis) {
    return false;
  }

  std::string login = user.login;
  const char* login_str = login.c_str();
  redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(redis, GET_USER_1S, login_str));
  if (!reply) {
    redisFree(redis);
    return false;
  }

  const char* userJson = reply->str;
  UserInfo linfo;
  if (parse_user_json(login, userJson, &linfo)) {
    if (user.password == linfo.GetPassword()) {
      *uinf = linfo;
      freeReplyObject(reply);
      redisFree(redis);
      return true;
    }
  }

  freeReplyObject(reply);
  redisFree(redis);
  return false;
}

RedisSubHandler::~RedisSubHandler() {}

RedisSub::RedisSub(RedisSubHandler* handler) : handler_(handler), stop_(false) {}

void RedisSub::setConfig(const redis_sub_configuration_t& config) {
  config_ = config;
}

void RedisSub::listen() {
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

    bool is_error_reply = lreply->type != REDIS_REPLY_ARRAY || lreply->elements != 3 ||
                          lreply->element[1]->type != REDIS_REPLY_STRING ||
                          lreply->element[2]->type != REDIS_REPLY_STRING;

    char* chn = lreply->element[1]->str;
    size_t chn_len = lreply->element[1]->len;
    char* msg = lreply->element[2]->str;
    size_t msg_len = lreply->element[2]->len;

    if (handler_) {
      handler_->handleMessage(std::string(chn, chn_len), std::string(msg, msg_len));
    }

    freeReplyObject(lreply);
  }

  freeReplyObject(reply);
  redisFree(redis_sub);
}

void RedisSub::stop() {
  stop_ = true;
}

bool RedisSub::publish_clients_state(const std::string& msg) {
  const char* channel = common::utils::c_strornull(config_.channel_clients_state);
  size_t chn_len = config_.channel_clients_state.length();
  return publish(channel, chn_len, msg.c_str(), msg.length());
}

bool RedisSub::publish_command_out(const char* msg, size_t msg_len) {
  const char* channel = common::utils::c_strornull(config_.channel_out);
  size_t chn_len = config_.channel_out.length();
  return publish(channel, chn_len, msg, msg_len);
}

bool RedisSub::publish(const char* chn, size_t chn_len, const char* msg, size_t msg_len) {
  if (!chn || chn_len == 0) {
    return false;
  }

  if (!msg || msg_len == 0) {
    return false;
  }

  redisContext* redis_sub = redis_connect(config_);
  if (!redis_sub) {
    return false;
  }

  void* rreply = redisCommand(redis_sub, "PUBLISH %s %s", chn, msg);
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
