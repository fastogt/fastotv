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

#include "runtime_channel_info.h"

#define RUNTIME_CHANNEL_INFO_CHANNEL_ID_FIELD "channel_id"
#define RUNTIME_CHANNEL_INFO_WATCHERS_FIELD "watchers"
#define RUNTIME_CHANNEL_INFO_CHAT_ENABLED_FIELD "chat_enabled"
#define RUNTIME_CHANNEL_INFO_MESSAGES_FIELD "messages"

namespace fastotv {

RuntimeChannelInfo::RuntimeChannelInfo()
    : channel_id_(invalid_epg_channel_id), watchers_(0), chat_enabled_(false), messages_() {}

RuntimeChannelInfo::RuntimeChannelInfo(stream_id channel_id, size_t watchers, bool chat_enabled, const messages_t& msgs)
    : channel_id_(channel_id), watchers_(watchers), chat_enabled_(chat_enabled), messages_(msgs) {}

RuntimeChannelInfo::~RuntimeChannelInfo() {}

bool RuntimeChannelInfo::IsValid() const {
  return channel_id_ != invalid_epg_channel_id;
}

common::Error RuntimeChannelInfo::SerializeImpl(serialize_type* deserialized) const {
  if (!IsValid()) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }

  json_object* obj = json_object_new_object();
  json_object* jmsgs = json_object_new_array();
  for (size_t i = 0; i < messages_.size(); ++i) {
    json_object_array_add(jmsgs, json_object_new_string(messages_[i].c_str()));
  }

  json_object_object_add(obj, RUNTIME_CHANNEL_INFO_CHANNEL_ID_FIELD, json_object_new_string(channel_id_.c_str()));
  json_object_object_add(obj, RUNTIME_CHANNEL_INFO_WATCHERS_FIELD, json_object_new_int(watchers_));
  json_object_object_add(obj, RUNTIME_CHANNEL_INFO_CHAT_ENABLED_FIELD, json_object_new_boolean(chat_enabled_));
  json_object_object_add(obj, RUNTIME_CHANNEL_INFO_MESSAGES_FIELD, jmsgs);
  *deserialized = obj;
  return common::Error();
}

common::Error RuntimeChannelInfo::DeSerialize(const serialize_type& serialized, value_type* obj) {
  if (!serialized || !obj) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }

  RuntimeChannelInfo inf;

  json_object* jwatchers = NULL;
  json_bool jwatchers_exists = json_object_object_get_ex(serialized, RUNTIME_CHANNEL_INFO_WATCHERS_FIELD, &jwatchers);
  if (jwatchers_exists) {
    inf.watchers_ = json_object_get_int64(jwatchers);
  }

  json_object* jcid = NULL;
  json_bool jcid_exists = json_object_object_get_ex(serialized, RUNTIME_CHANNEL_INFO_CHANNEL_ID_FIELD, &jcid);
  if (!jcid_exists) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }
  stream_id cid = json_object_get_string(jcid);
  if (cid == invalid_stream_id) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }
  inf.channel_id_ = cid;

  json_object* jchat_enabled = NULL;
  json_bool jchat_enabled_exists =
      json_object_object_get_ex(serialized, RUNTIME_CHANNEL_INFO_CHAT_ENABLED_FIELD, &jchat_enabled);
  if (jchat_enabled_exists) {
    inf.chat_enabled_ = json_object_get_boolean(jchat_enabled);
  }

  json_object* jmsgs = NULL;
  json_bool jmsgs_exists = json_object_object_get_ex(serialized, RUNTIME_CHANNEL_INFO_MESSAGES_FIELD, &jmsgs);
  if (jmsgs_exists) {
    messages_t msgs;
    size_t len = json_object_array_length(jmsgs);
    for (size_t i = 0; i < len; ++i) {
      json_object* jmess = json_object_array_get_idx(jmsgs, i);
      msgs.push_back(json_object_get_string(jmess));
    }
    inf.messages_ = msgs;
  }

  *obj = inf;
  return common::Error();
}

bool RuntimeChannelInfo::Equals(const RuntimeChannelInfo& inf) const {
  return channel_id_ == inf.channel_id_ && watchers_ == inf.watchers_ && chat_enabled_ == inf.chat_enabled_ &&
         messages_ == inf.messages_;
}

}  // namespace fastotv
