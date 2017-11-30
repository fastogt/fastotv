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

#define PROGRAMME_INFO_CHANNEL_FIELD "channel"
#define PROGRAMME_INFO_START_FIELD "start"
#define PROGRAMME_INFO_STOP_FIELD "stop"
#define PROGRAMME_INFO_TITLE_FIELD "title"

namespace fastotv {

ProgrammeInfo::ProgrammeInfo() : channel_(invalid_stream_id), start_time_(0), stop_time_(0), title_() {}

ProgrammeInfo::ProgrammeInfo(stream_id channel, timestamp_t start, timestamp_t stop, const std::string& title)
    : channel_(channel), start_time_(start), stop_time_(stop), title_(title) {}

bool ProgrammeInfo::IsValid() const {
  return channel_ != invalid_stream_id && !title_.empty();
}

common::Error ProgrammeInfo::SerializeFields(json_object* obj) const {
  if (!IsValid()) {
    return common::make_error_inval();
  }

  json_object_object_add(obj, PROGRAMME_INFO_CHANNEL_FIELD, json_object_new_string(channel_.c_str()));
  json_object_object_add(obj, PROGRAMME_INFO_START_FIELD, json_object_new_int64(start_time_));
  json_object_object_add(obj, PROGRAMME_INFO_STOP_FIELD, json_object_new_int64(stop_time_));
  json_object_object_add(obj, PROGRAMME_INFO_TITLE_FIELD, json_object_new_string(title_.c_str()));
  return common::Error();
}

common::Error ProgrammeInfo::DeSerialize(const serialize_type& serialized, ProgrammeInfo* obj) {
  if (!serialized || !obj) {
    return common::make_error_inval();
  }

  json_object* jchannel = NULL;
  json_bool jchannel_exists = json_object_object_get_ex(serialized, PROGRAMME_INFO_CHANNEL_FIELD, &jchannel);
  if (!jchannel_exists) {
    return common::make_error_inval();
  }

  json_object* jstart = NULL;
  json_bool jstart_exists = json_object_object_get_ex(serialized, PROGRAMME_INFO_START_FIELD, &jstart);
  if (!jstart_exists) {
    return common::make_error_inval();
  }

  json_object* jstop = NULL;
  json_bool jstop_exists = json_object_object_get_ex(serialized, PROGRAMME_INFO_STOP_FIELD, &jstop);
  if (!jstop_exists) {
    return common::make_error_inval();
  }

  json_object* jtitle = NULL;
  json_bool jtitle_exists = json_object_object_get_ex(serialized, PROGRAMME_INFO_TITLE_FIELD, &jtitle);
  if (!jtitle_exists) {
    return common::make_error_inval();
  }

  fastotv::ProgrammeInfo prog(json_object_get_string(jchannel), json_object_get_int64(jstart),
                              json_object_get_int64(jstop), json_object_get_string(jtitle));
  *obj = prog;
  return common::Error();
}

void ProgrammeInfo::SetChannel(stream_id channel) {
  channel_ = channel;
}

stream_id ProgrammeInfo::GetChannel() const {
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

std::string ProgrammeInfo::GetTitle() const {
  return title_;
}

bool ProgrammeInfo::Equals(const ProgrammeInfo& prog) const {
  return channel_ == prog.channel_ && start_time_ == prog.start_time_ && stop_time_ == prog.stop_time_ &&
         title_ == prog.title_;
}

}  // namespace fastotv
