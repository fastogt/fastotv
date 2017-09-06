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

#include <string>  // for string

#include <common/error.h>    // for Error
#include <common/macros.h>   // for WARN_UNUSED_RESULT
#include <common/uri/url.h>  // for Uri

#include "client_server_types.h"
#include "epg_info.h"
#include "serializer/json_serializer.h"

namespace fastotv {

class ChannelInfo : public JsonSerializer<ChannelInfo> {
 public:
  ChannelInfo();
  ChannelInfo(const EpgInfo& epg, bool enable_audio, bool enable_video);

  bool IsValid() const;
  common::uri::Url GetUrl() const;
  std::string GetName() const;
  stream_id GetId() const;
  EpgInfo GetEpg() const;

  bool IsEnableAudio() const;
  bool IsEnableVideo() const;

  static common::Error DeSerialize(const serialize_type& serialized, value_type* obj) WARN_UNUSED_RESULT;

  bool Equals(const ChannelInfo& url) const;

 protected:
  common::Error SerializeImpl(serialize_type* deserialized) const override;

 private:
  EpgInfo epg_;

  bool enable_audio_;
  bool enable_video_;
};

inline bool operator==(const ChannelInfo& left, const ChannelInfo& right) {
  return left.Equals(right);
}

}  // namespace fastotv
