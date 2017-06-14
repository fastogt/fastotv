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

#include "programme_info.h"

/*
<programme start="20170613010000 +0000" stop="20170613020000 +0000" channel="FoxNews.us">
  <title lang="en">The Five</title>
  <desc lang="en">The Five covers the hot topics that have everyone talking, and the viewpoints of five voices that will
have everyone listening!</desc>
  <category lang="en">Talk</category>
  <category lang="en">Current Affairs</category>
  <star-rating>
    <value>8.8</value>
  </star-rating>
</programme>
*/

#include <stddef.h>  // for NULL
#include <string>    // for string

#include <common/convert2string.h>
#include <common/sprintf.h>

#define PROGRAMME_INFO_CHANNEL_FIELD "channel"
#define PROGRAMME_INFO_START_FIELD "start"
#define PROGRAMME_INFO_STOP_FIELD "stop"
#define PROGRAMME_INFO_TITLE_FIELD "title"

namespace fasto {
namespace fastotv {

ProgrammeInfo::ProgrammeInfo() : channel_(invalid_epg_channel_id), start_time_(0), stop_time_(0), title_() {}

ProgrammeInfo::ProgrammeInfo(epg_channel_id channel, timestamp_t start, timestamp_t stop, const std::string& title)
    : channel_(channel), start_time_(start), stop_time_(stop), title_(title) {}

bool ProgrammeInfo::IsValid() const {
  return channel_ != invalid_stream_id && !title_.empty();
}

common::Error ProgrammeInfo::SerializeImpl(serialize_type* deserialized) const {
  if (!IsValid()) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  json_object* obj = json_object_new_object();
  json_object_object_add(obj, PROGRAMME_INFO_CHANNEL_FIELD, json_object_new_string(channel_.c_str()));
  json_object_object_add(obj, PROGRAMME_INFO_START_FIELD, json_object_new_int64(start_time_));
  json_object_object_add(obj, PROGRAMME_INFO_STOP_FIELD, json_object_new_int64(stop_time_));
  json_object_object_add(obj, PROGRAMME_INFO_TITLE_FIELD, json_object_new_string(title_.c_str()));

  *deserialized = obj;
  return common::Error();
}

common::Error ProgrammeInfo::DeSerialize(const serialize_type& serialized, value_type* obj) {
  if (!serialized || !obj) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  json_object* jchannel = NULL;
  json_bool jchannel_exists = json_object_object_get_ex(serialized, PROGRAMME_INFO_CHANNEL_FIELD, &jchannel);
  if (!jchannel_exists) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  json_object* jstart = NULL;
  json_bool jstart_exists = json_object_object_get_ex(serialized, PROGRAMME_INFO_START_FIELD, &jstart);
  if (!jstart_exists) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  json_object* jstop = NULL;
  json_bool jstop_exists = json_object_object_get_ex(serialized, PROGRAMME_INFO_STOP_FIELD, &jstop);
  if (!jstop_exists) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  json_object* jtitle = NULL;
  json_bool jtitle_exists = json_object_object_get_ex(serialized, PROGRAMME_INFO_TITLE_FIELD, &jtitle);
  if (!jtitle_exists) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  fasto::fastotv::ProgrammeInfo prog(json_object_get_string(jchannel), json_object_get_int64(jstart),
                                     json_object_get_int64(jstop), json_object_get_string(jtitle));
  *obj = prog;
  return common::Error();
}

void ProgrammeInfo::SetChannel(epg_channel_id channel) {
  channel_ = channel;
}

epg_channel_id ProgrammeInfo::GetChannel() const {
  return channel_;
}

void ProgrammeInfo::SetStart(timestamp_t start) {
  start_time_ = start;
}

timestamp_t ProgrammeInfo::GetStart() const {
  return start_time_;
}

void ProgrammeInfo::SetStop(timestamp_t stop) {
  stop_time_ = stop;
}

timestamp_t ProgrammeInfo::GetStop() const {
  return stop_time_;
}

void ProgrammeInfo::SetTitle(const std::string& title) {
  title_ = title;
}

epg_channel_id ProgrammeInfo::GetTitle() const {
  return title_;
}

bool ProgrammeInfo::Equals(const ProgrammeInfo& prog) const {
  return channel_ == prog.channel_ && start_time_ == prog.start_time_ && stop_time_ == prog.stop_time_ &&
         title_ == prog.title_;
}

}  // namespace fastotv
}  // namespace fasto
