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

#include "server/redis/redis_storage.h"

#include <stddef.h>  // for NULL
#include <string>    // for string

#include <hiredis/hiredis.h>  // for redisFree, freeR...

#include <common/logger.h>  // for COMPACT_LOG_ERROR
#include <common/utils.h>

#include "auth_info.h"  // for AuthInfo

#include "third-party/json-c/json-c/json_object.h"   // for json_object_put
#include "third-party/json-c/json-c/json_tokener.h"  // for json_tokener_parse

#include "server/redis/redis_connect.h"

#define GET_USER_1E "GET %s"

#define ID_FIELD "id"

namespace fasto {
namespace fastotv {
namespace server {
namespace {

common::Error parse_user_json(const char* user_json, user_id_t* out_uid, UserInfo* out_info) {
  if (!user_json || !out_uid || !out_info) {
    return common::make_inval_error_value(common::ErrorValue::E_ERROR);
  }

  json_object* obj = json_tokener_parse(user_json);
  if (!obj) {
    return common::make_error_value("Can't parse database field", common::ErrorValue::E_ERROR);
  }

  json_object* jid = NULL;
  json_bool jid_exists = json_object_object_get_ex(obj, ID_FIELD, &jid);  // mongodb id
  if (!jid_exists) {
    json_object_put(obj);
    return common::make_error_value("Can't parse database field", common::ErrorValue::E_ERROR);
  }

  UserInfo uinf;
  common::Error err = UserInfo::DeSerialize(obj, &uinf);
  if (err && err->IsError()) {
    json_object_put(obj);
    return err;
  }

  *out_uid = json_object_get_string(jid);
  *out_info = uinf;
  json_object_put(obj);
  return common::Error();
}

}  // namespace

RedisStorage::RedisStorage() : config_() {}

void RedisStorage::SetConfig(const RedisConfig& config) {
  config_ = config;
}

common::Error RedisStorage::FindUserAuth(const AuthInfo& user, user_id_t* uid) const {
  UserInfo uinf;
  return FindUser(user, uid, &uinf);
}

common::Error RedisStorage::FindUser(const AuthInfo& user, user_id_t* uid, UserInfo* uinf) const {
  if (!user.IsValid() || !uid || !uinf) {
    return common::make_inval_error_value(common::ErrorValue::E_ERROR);
  }

  redisContext* redis = NULL;
  common::Error err = redis_connect(config_, &redis);
  if (err && err->IsError()) {
    return err;
  }

  std::string login = user.GetLogin();
  const char* login_str = login.c_str();
  redisReply* reply = reinterpret_cast<redisReply*>(redisCommand(redis, GET_USER_1E, login_str));
  if (!reply) {
    redisFree(redis);
    return common::make_error_value("User not found", common::ErrorValue::E_ERROR);
  }

  const char* user_json = reply->str;
  UserInfo linfo;
  user_id_t luid;
  err = parse_user_json(user_json, &luid, &linfo);
  if (err && err->IsError()) {
    freeReplyObject(reply);
    redisFree(redis);
    return err;
  }

  AuthInfo lauth = linfo.GetAuthInfo();
  if (user.GetPassword() != lauth.GetPassword()) {
    freeReplyObject(reply);
    redisFree(redis);
    return common::make_error_value("Password missmatch", common::ErrorValue::E_ERROR);
  }

  *uid = luid;
  *uinf = linfo;
  freeReplyObject(reply);
  redisFree(redis);
  return common::Error();
}

}  // namespace server
}  // namespace fastotv
}  // namespace fasto
