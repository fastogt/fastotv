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
#include "programme_info.h"

#include "serializer/json_serializer.h"

namespace fastotv {

class EpgInfo : public JsonSerializer<EpgInfo> {
 public:
  typedef std::vector<ProgrammeInfo> programs_t;
  EpgInfo();
  EpgInfo(stream_id id, const common::uri::Url& uri, const std::string& name);  // required args

  bool IsValid() const;
  bool FindProgrammeByTime(timestamp_t time, ProgrammeInfo* inf) const;

  void SetUrl(const common::uri::Url& url);
  common::uri::Url GetUrl() const;

  void SetDisplayName(const std::string& name);
  std::string GetDisplayName() const;

  void SetChannelId(stream_id ch);
  stream_id GetChannelId() const;

  void SetIconUrl(const common::uri::Url& url);
  common::uri::Url GetIconUrl() const;

  void SetPrograms(const programs_t& progs);
  programs_t GetPrograms() const;

  static common::Error DeSerialize(const serialize_type& serialized, value_type* obj) WARN_UNUSED_RESULT;

  bool Equals(const EpgInfo& url) const;

  static const common::uri::Url& GetUnknownIconUrl();
  static bool IsUnknownIconUrl(const common::uri::Url& url);

 protected:
  common::Error SerializeImpl(serialize_type* deserialized) const override;

 private:
  stream_id channel_id_;
  common::uri::Url uri_;
  std::string display_name_;
  common::uri::Url icon_src_;
  programs_t programs_;
};

inline bool operator==(const EpgInfo& left, const EpgInfo& right) {
  return left.Equals(right);
}

}  // namespace fastotv
