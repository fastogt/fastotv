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

#pragma once

#include <string>

#include <common/serializer/json_serializer.h>

#include "client_server_types.h"

namespace fastotv {

class ProgrammeInfo : public common::serializer::JsonSerializer<ProgrammeInfo> {
 public:
  ProgrammeInfo();
  ProgrammeInfo(stream_id id, timestamp_t start_time, timestamp_t stop_time, const std::string& title);

  bool IsValid() const;

  void SetChannel(stream_id channel);
  stream_id GetChannel() const;

  void SetStart(timestamp_t start);
  timestamp_t GetStart() const;

  void SetStop(timestamp_t stop);
  timestamp_t GetStop() const;

  void SetTitle(const std::string& title);
  std::string GetTitle() const;

  bool Equals(const ProgrammeInfo& prog) const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* deserialized) const override;

 private:
  stream_id channel_;
  timestamp_t start_time_;  // utc time
  timestamp_t stop_time_;   // utc time
  std::string title_;
};

inline bool operator==(const ProgrammeInfo& lhs, const ProgrammeInfo& rhs) {
  return lhs.Equals(rhs);
}

inline bool operator!=(const ProgrammeInfo& x, const ProgrammeInfo& y) {
  return !(x == y);
}

}  // namespace fastotv
