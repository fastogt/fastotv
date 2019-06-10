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

#include <vector>

#include <common/serializer/json_serializer.h>

#include "client_server_types.h"

namespace fastotv {

class RuntimeChannelLiteInfo : public common::serializer::JsonSerializer<RuntimeChannelLiteInfo> {
 public:
  RuntimeChannelLiteInfo();
  RuntimeChannelLiteInfo(stream_id channel_id);
  ~RuntimeChannelLiteInfo();

  bool IsValid() const;

  void SetChannelID(stream_id sid);
  stream_id GetChannelID() const;

  bool Equals(const RuntimeChannelLiteInfo& inf) const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* deserialized) const override;

 private:
  stream_id channel_id_;
};

inline bool operator==(const RuntimeChannelLiteInfo& left, const RuntimeChannelLiteInfo& right) {
  return left.Equals(right);
}

class RuntimeChannelInfo : public RuntimeChannelLiteInfo {
 public:
  typedef RuntimeChannelLiteInfo base_class;
  RuntimeChannelInfo();
  RuntimeChannelInfo(stream_id channel_id, size_t watchers, ChannelType type);
  ~RuntimeChannelInfo() override;

  void SetWatchersCount(size_t count);
  size_t GetWatchersCount() const;

  void SetChannelType(ChannelType ct);
  ChannelType GetChannelType() const;

  bool Equals(const RuntimeChannelInfo& inf) const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* deserialized) const override;

 private:
  size_t watchers_;
  ChannelType type_;
};

inline bool operator==(const RuntimeChannelInfo& left, const RuntimeChannelInfo& right) {
  return left.Equals(right);
}

inline bool operator!=(const RuntimeChannelInfo& x, const RuntimeChannelInfo& y) {
  return !(x == y);
}

}  // namespace fastotv
