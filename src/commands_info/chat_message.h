/*  Copyright (C) 2014-2019 FastoGT. All right reserved.

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

#pragma once

#include <string>

#include <common/serializer/json_serializer.h>

#include "client_server_types.h"

// {"channel" : "1234", "login" : "atopilski@gmail.com", "message" : "leave the channel test", "type" : 0}
// {"channel" : "1234", "login" : "atopilski@gmail.com", "message" : "Hello", "type" : 1}

namespace fastotv {

class ChatMessage : public common::serializer::JsonSerializer<ChatMessage> {
 public:
  enum Type { CONTROL = 0, MESSAGE };
  ChatMessage();
  ChatMessage(stream_id channel, login_t login, const std::string& message, Type type);

  bool IsValid() const;

  void SetMessage(const std::string& msg);
  std::string GetMessage() const;

  void SetChannelID(stream_id sid);
  stream_id GetChannelID() const;

  void SetLogin(login_t login);
  login_t GetLogin() const;

  Type GetType() const;

  bool Equals(const ChatMessage& inf) const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* deserialized) const override;

 private:
  stream_id channel_id_;
  login_t login_;
  std::string message_;
  Type type_;
};

ChatMessage MakeEnterMessage(stream_id sid, login_t login);
ChatMessage MakeLeaveMessage(stream_id sid, login_t login);

bool IsEnterMessage(const ChatMessage& msg);
bool IsLeaveMessage(const ChatMessage& msg);

inline bool operator==(const ChatMessage& left, const ChatMessage& right) {
  return left.Equals(right);
}

inline bool operator!=(const ChatMessage& x, const ChatMessage& y) {
  return !(x == y);
}

}  // namespace fastotv
