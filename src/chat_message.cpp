/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

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

#include "chat_message.h"

#include <common/sprintf.h>

#define CHAT_MESSAGE_CHANNEL_ID_FIELD "channel_id"
#define CHAT_MESSAGE_LOGIN_FIELD "login"
#define CHAT_MESSAGE_MESSAGE_FIELD "message"
#define CHAT_MESSAGE_TYPE_FIELD "type"

#define ENTER_MESSAGE "enter to chat."
#define ENTER_MESSAGE_TEMPLATE "%s " ENTER_MESSAGE

#define LEAVE_MESSAGE "leave from chat."
#define LEAVE_MESSAGE_TEMPLATE "%s " LEAVE_MESSAGE

namespace fastotv {

ChatMessage::ChatMessage() : channel_id_(invalid_stream_id), login_(), message_(), type_(CONTROL) {}

ChatMessage::ChatMessage(stream_id channel, login_t login, const std::string& message, Type type)
    : channel_id_(channel), login_(login), message_(message), type_(type) {}

bool ChatMessage::IsValid() const {
  return channel_id_ != invalid_stream_id && !login_.empty() && !message_.empty();
}

void ChatMessage::SetMessage(const std::string& msg) {
  message_ = msg;
}

std::string ChatMessage::GetMessage() const {
  return message_;
}

void ChatMessage::SetChannelId(stream_id sid) {
  channel_id_ = sid;
}

stream_id ChatMessage::GetChannelId() const {
  return channel_id_;
}

void ChatMessage::SetLogin(login_t login) {
  login_ = login;
}

login_t ChatMessage::GetLogin() const {
  return login_;
}

ChatMessage::Type ChatMessage::GetType() const {
  return type_;
}

common::Error ChatMessage::SerializeFields(json_object* obj) const {
  if (!IsValid()) {
    return common::make_error_inval();
  }

  json_object_object_add(obj, CHAT_MESSAGE_CHANNEL_ID_FIELD, json_object_new_string(channel_id_.c_str()));
  json_object_object_add(obj, CHAT_MESSAGE_LOGIN_FIELD, json_object_new_string(login_.c_str()));
  json_object_object_add(obj, CHAT_MESSAGE_MESSAGE_FIELD, json_object_new_string(message_.c_str()));
  json_object_object_add(obj, CHAT_MESSAGE_TYPE_FIELD, json_object_new_int(type_));
  return common::Error();
}

common::Error ChatMessage::DeSerialize(const serialize_type& serialized, ChatMessage* obj) {
  if (!serialized || !obj) {
    return common::make_error_inval();
  }

  ChatMessage msg;
  json_object* jchan = NULL;
  json_bool jchan_exists = json_object_object_get_ex(serialized, CHAT_MESSAGE_CHANNEL_ID_FIELD, &jchan);
  if (!jchan_exists) {
    return common::make_error_inval();
  }
  const stream_id chan = json_object_get_string(jchan);
  if (chan == invalid_stream_id) {
    return common::make_error_inval();
  }
  msg.channel_id_ = chan;

  json_object* jlogin = NULL;
  json_bool jlogin_exists = json_object_object_get_ex(serialized, CHAT_MESSAGE_LOGIN_FIELD, &jlogin);
  if (!jlogin_exists) {
    return common::make_error_inval();
  }
  const std::string login = json_object_get_string(jlogin);
  if (login.empty()) {
    return common::make_error_inval();
  }
  msg.login_ = login;

  json_object* jmessage = NULL;
  json_bool jmessage_exists = json_object_object_get_ex(serialized, CHAT_MESSAGE_MESSAGE_FIELD, &jmessage);
  if (!jmessage_exists) {
    return common::make_error_inval();
  }
  const std::string message = json_object_get_string(jmessage);
  if (message.empty()) {
    return common::make_error_inval();
  }
  msg.message_ = message;

  json_object* jtype = NULL;
  json_bool jtype_exists = json_object_object_get_ex(serialized, CHAT_MESSAGE_TYPE_FIELD, &jtype);
  if (jtype_exists) {
    msg.type_ = static_cast<Type>(json_object_get_int(jtype));
  }

  *obj = msg;
  return common::Error();
}

ChatMessage MakeEnterMessage(stream_id sid, login_t login) {
  return ChatMessage(sid, login, common::MemSPrintf(ENTER_MESSAGE_TEMPLATE, login), ChatMessage::CONTROL);
}

ChatMessage MakeLeaveMessage(stream_id sid, login_t login) {
  return ChatMessage(sid, login, common::MemSPrintf(LEAVE_MESSAGE_TEMPLATE, login), ChatMessage::CONTROL);
}

bool IsEnterMessage(const ChatMessage& msg) {
  if (msg.GetType() != ChatMessage::CONTROL) {
    return false;
  }

  std::string msg_str = msg.GetMessage();
  return msg_str.find(ENTER_MESSAGE) != std::string::npos;
}

bool IsLeaveMessage(const ChatMessage& msg) {
  if (msg.GetType() != ChatMessage::CONTROL) {
    return false;
  }

  std::string msg_str = msg.GetMessage();
  return msg_str.find(LEAVE_MESSAGE) != std::string::npos;
}

bool ChatMessage::Equals(const ChatMessage& inf) const {
  return channel_id_ == inf.channel_id_ && login_ == inf.login_ && message_ == inf.message_ && type_ == inf.type_;
}

}  // namespace fastotv
