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

#include "url.h"

#include "third-party/json-c/json-c/json.h"  // for json_object_...

#define ID_FIELD "id"
#define URL_FIELD "url"
#define NAME_FIELD "name"
#define VIDEO_ENABLE_FIELD "video"
#define AUDIO_ENABLE_FIELD "audio"

namespace fasto {
namespace fastotv {
Url::Url() : id_(invalid_stream_id), uri_(), name_(), enable_audio_(true), enable_video_(true) {}

Url::Url(stream_id id,
         const common::uri::Uri& uri,
         const std::string& name,
         bool enable_audio,
         bool enable_video)
    : id_(id), uri_(uri), name_(name), enable_audio_(enable_audio), enable_video_(enable_video) {}

bool Url::IsValid() const {
  return id_ != invalid_stream_id && uri_.IsValid();
}

common::uri::Uri Url::GetUrl() const {
  return uri_;
}

std::string Url::GetName() const {
  return name_;
}

stream_id Url::Id() const {
  return id_;
}

bool Url::IsEnableAudio() const {
  return enable_audio_;
}

bool Url::IsEnableVideo() const {
  return enable_video_;
}

common::Error Url::Serialize(serialize_type* deserialized) const {
  json_object* obj = json_object_new_object();
  json_object_object_add(obj, ID_FIELD, json_object_new_string(id_.c_str()));
  const std::string url_str = uri_.Url();
  json_object_object_add(obj, URL_FIELD, json_object_new_string(url_str.c_str()));
  json_object_object_add(obj, NAME_FIELD, json_object_new_string(name_.c_str()));
  json_object_object_add(obj, AUDIO_ENABLE_FIELD, json_object_new_boolean(enable_audio_));
  json_object_object_add(obj, VIDEO_ENABLE_FIELD, json_object_new_boolean(enable_video_));
  *deserialized = obj;
  return common::Error();
}

common::Error Url::DeSerialize(const serialize_type& serialized, value_type* obj) {
  json_object* jid = NULL;
  json_bool jid_exists = json_object_object_get_ex(serialized, ID_FIELD, &jid);
  if (!jid_exists) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  json_object* jurl = NULL;
  json_bool jurls_exists = json_object_object_get_ex(serialized, URL_FIELD, &jurl);
  if (!jurls_exists) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  json_object* jname = NULL;
  json_bool jname_exists = json_object_object_get_ex(serialized, NAME_FIELD, &jname);
  if (!jname_exists) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  bool enable_audio = true;
  json_object* jenable_audio = NULL;
  json_bool jenable_audio_exists =
      json_object_object_get_ex(serialized, AUDIO_ENABLE_FIELD, &jenable_audio);
  if (jenable_audio_exists) {
    enable_audio = json_object_get_boolean(jenable_audio);
  }

  bool enable_video = true;
  json_object* jdisable_video = NULL;
  json_bool jdisable_video_exists =
      json_object_object_get_ex(serialized, VIDEO_ENABLE_FIELD, &jdisable_video);
  if (jdisable_video_exists) {
    enable_video = !json_object_get_boolean(jdisable_video);
  }

  fasto::fastotv::Url url(json_object_get_string(jid),
                          common::uri::Uri(json_object_get_string(jurl)),
                          json_object_get_string(jname), enable_audio, enable_video);
  if (!url.IsValid()) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  *obj = url;
  return common::Error();
}
}
}
