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

namespace fasto {
namespace fastotv {
Url::Url() : id_(invalid_stream_id), uri_(), name_() {
}

Url::Url(stream_id id, const common::uri::Uri& uri, const std::string& name)
    : id_(id), uri_(uri), name_(name) {
}

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

common::Error Url::Serialize(serialize_type* deserialized) const {
  json_object* obj = json_object_new_object();
  json_object_object_add(obj, ID_FIELD, json_object_new_string(id_.c_str()));
  const std::string url_str = uri_.Url();
  json_object_object_add(obj, URL_FIELD, json_object_new_string(url_str.c_str()));
  json_object_object_add(obj, NAME_FIELD, json_object_new_string(name_.c_str()));
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

  fasto::fastotv::Url url(json_object_get_string(jid),
                          common::uri::Uri(json_object_get_string(jurl)),
                          json_object_get_string(jname));
  if (!url.IsValid()) {
    return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
  }

  *obj = url;
  return common::Error();
}
}
}
