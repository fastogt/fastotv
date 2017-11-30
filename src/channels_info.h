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

#pragma once

#include "channel_info.h"

#include "serializer/json_serializer.h"

namespace fastotv {

class ChannelsInfo : public JsonSerializer {
 public:
  typedef std::vector<ChannelInfo> channels_t;
  ChannelsInfo();

  static common::Error DeSerialize(const serialize_type& serialized, ChannelsInfo* obj) WARN_UNUSED_RESULT;

  void AddChannel(const ChannelInfo& channel);
  channels_t GetChannels() const;

  size_t GetSize() const;
  bool IsEmpty() const;

  bool Equals(const ChannelsInfo& chan) const;

 protected:
  virtual common::Error SerializeImpl(serialize_type* deserialized) const override;

 private:
  channels_t channels_;
};

inline bool operator==(const ChannelsInfo& lhs, const ChannelsInfo& rhs) {
  return lhs.Equals(rhs);
}

inline bool operator!=(const ChannelsInfo& x, const ChannelsInfo& y) {
  return !(x == y);
}

}  // namespace fastotv
