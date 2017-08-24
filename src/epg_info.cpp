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

#include "epg_info.h"

/*
<channel id="id">
  <display-name lang="ru"></display-name>
  <url>http://www.example.com</url>
  <icon src="http://example.com/x.png"/>
</channel>
*/

#define EPG_INFO_ID_FIELD "id"
#define EPG_INFO_URL_FIELD "url"
#define EPG_INFO_NAME_FIELD "display_name"
#define EPG_INFO_ICON_FIELD "icon"
#define EPG_INFO_PROGRAMS_FIELD "programs"

namespace fastotv {

EpgInfo::EpgInfo()
    : id_(invalid_epg_channel_id), uri_(), display_name_(), icon_src_(GetUnknownIconUrl()), programs_() {}

EpgInfo::EpgInfo(epg_channel_id id, const common::uri::Uri& uri, const std::string& name)
    : id_(id), uri_(uri), display_name_(name), icon_src_(GetUnknownIconUrl()), programs_() {}

bool EpgInfo::IsValid() const {
  return id_ != invalid_epg_channel_id && uri_.IsValid() && !display_name_.empty();
}

bool EpgInfo::FindProgrammeByTime(timestamp_t time, ProgrammeInfo* inf) const {
  if (!inf || !IsValid()) {
    return false;
  }

  for (size_t i = 0; i < programs_.size(); ++i) {
    ProgrammeInfo pr = programs_[i];
    if (time >= pr.GetStart() && time <= pr.GetStop()) {
      *inf = pr;
      return true;
    }
  }

  return false;
}

void EpgInfo::SetUrl(const common::uri::Uri& url) {
  uri_ = url;
}

common::uri::Uri EpgInfo::GetUrl() const {
  return uri_;
}

void EpgInfo::SetDisplayName(const std::string& name) {
  display_name_ = name;
}

std::string EpgInfo::GetDisplayName() const {
  return display_name_;
}

void EpgInfo::SetId(epg_channel_id id) {
  id_ = id;
}

epg_channel_id EpgInfo::GetId() const {
  return id_;
}

void EpgInfo::SetIconUrl(const common::uri::Uri& url) {
  icon_src_ = url;
}

void EpgInfo::SetPrograms(const programs_t& progs) {
  programs_ = progs;
}

EpgInfo::programs_t EpgInfo::GetPrograms() const {
  return programs_;
}

common::uri::Uri EpgInfo::GetIconUrl() const {
  return icon_src_;
}

common::Error EpgInfo::SerializeImpl(serialize_type* deserialized) const {
  if (!IsValid()) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }

  json_object* obj = json_object_new_object();
  json_object_object_add(obj, EPG_INFO_ID_FIELD, json_object_new_string(id_.c_str()));
  const std::string url_str = uri_.GetUrl();
  json_object_object_add(obj, EPG_INFO_URL_FIELD, json_object_new_string(url_str.c_str()));
  json_object_object_add(obj, EPG_INFO_NAME_FIELD, json_object_new_string(display_name_.c_str()));
  const std::string icon_url_str = icon_src_.GetUrl();
  json_object_object_add(obj, EPG_INFO_ICON_FIELD, json_object_new_string(icon_url_str.c_str()));

  json_object* jprograms = json_object_new_array();
  for (ProgrammeInfo prog : programs_) {
    json_object* jprog = NULL;
    common::Error err = prog.Serialize(&jprog);
    if (err && err->IsError()) {
      continue;
    }
    json_object_array_add(jprograms, jprog);
  }
  json_object_object_add(obj, EPG_INFO_PROGRAMS_FIELD, jprograms);

  *deserialized = obj;
  return common::Error();
}

common::Error EpgInfo::DeSerialize(const serialize_type& serialized, value_type* obj) {
  if (!serialized || !obj) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }

  json_object* jid = NULL;
  json_bool jid_exists = json_object_object_get_ex(serialized, EPG_INFO_ID_FIELD, &jid);
  if (!jid_exists) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }
  stream_id id = json_object_get_string(jid);
  if (id == invalid_stream_id) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }

  json_object* jurl = NULL;
  json_bool jurls_exists = json_object_object_get_ex(serialized, EPG_INFO_URL_FIELD, &jurl);
  if (!jurls_exists) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }

  common::uri::Uri uri(json_object_get_string(jurl));
  if (!uri.IsValid()) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }

  json_object* jname = NULL;
  json_bool jname_exists = json_object_object_get_ex(serialized, EPG_INFO_NAME_FIELD, &jname);
  if (!jname_exists) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }

  std::string name = json_object_get_string(jname);
  if (name.empty()) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }

  fastotv::EpgInfo url(id, uri, name);
  if (!url.IsValid()) {
    return common::make_inval_error_value(common::Value::E_ERROR);
  }

  json_object* jurl_icon = NULL;
  json_bool jurl_icon_exists = json_object_object_get_ex(serialized, EPG_INFO_ICON_FIELD, &jurl_icon);
  if (jurl_icon_exists) {
    url.icon_src_ = common::uri::Uri(json_object_get_string(jurl_icon));
  }

  json_object* jprogs = NULL;
  json_bool jprogs_exists = json_object_object_get_ex(serialized, EPG_INFO_PROGRAMS_FIELD, &jprogs);
  if (jprogs_exists) {
    programs_t progs;
    size_t len = json_object_array_length(jprogs);
    for (size_t i = 0; i < len; ++i) {
      json_object* jprog = json_object_array_get_idx(jprogs, i);
      ProgrammeInfo prog;
      common::Error err = ProgrammeInfo::DeSerialize(jprog, &prog);
      if (err && err->IsError()) {
        continue;
      }
      progs.push_back(prog);
    }
    url.programs_ = progs;
  }

  *obj = url;
  return common::Error();
}

bool EpgInfo::Equals(const EpgInfo& url) const {
  return id_ == url.id_ && uri_ == url.uri_ && display_name_ == url.display_name_;
}

const common::uri::Uri& EpgInfo::GetUnknownIconUrl() {
  static const common::uri::Uri url(UNKNOWN_ICON_URI);
  return url;
}

bool EpgInfo::IsUnknownIconUrl(const common::uri::Uri& url) {
  return url == GetUnknownIconUrl();
}

}  // namespace fastotv
