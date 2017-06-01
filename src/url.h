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

#include <limits>

#include <common/url.h>

#include "client_server_types.h"
#include "serializer/json_serializer.h"

namespace fasto {
namespace fastotv {

class Url : public JsonSerializer<Url> {
 public:
  Url();
  Url(stream_id id,
      const common::uri::Uri& uri,
      const std::string& name,
      bool enable_audio,
      bool enable_video);

  bool IsValid() const;
  common::uri::Uri GetUrl() const;
  std::string GetName() const;
  stream_id GetId() const;

  bool IsEnableAudio() const;
  bool IsEnableVideo() const;

  common::Error Serialize(serialize_type* deserialized) const WARN_UNUSED_RESULT;
  static common::Error DeSerialize(const serialize_type& serialized,
                                   value_type* obj) WARN_UNUSED_RESULT;

  bool Equals(const Url& url) const;

 private:
  stream_id id_;
  common::uri::Uri uri_;
  std::string name_;

  bool enable_audio_;
  bool enable_video_;
};

inline bool operator==(const Url& left, const Url& right) {
  return left.Equals(right);
}
}  // namespace fastotv
}  // namespace fasto
