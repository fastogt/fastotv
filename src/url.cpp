#include "url.h"

#include "third-party/json-c/json-c/json.h"  // for json_object_...

#define ID_FIELD "id"
#define URL_FIELD "url"
#define NAME_FIELD "name"

Url::Url(const std::string& json_data) : id_(core::invalid_stream_id), uri_(), name_() {
  json_object* obj = json_tokener_parse(json_data.c_str());
  if (!obj) {
    return;
  }

  json_object* jid = NULL;
  json_bool jid_exists = json_object_object_get_ex(obj, ID_FIELD, &jid);
  if (!jid_exists) {
    json_object_put(obj);
    return;
  }

  json_object* jurl = NULL;
  json_bool jurls_exists = json_object_object_get_ex(obj, URL_FIELD, &jurl);
  if (!jurls_exists) {
    json_object_put(obj);
    return;
  }

  json_object* jname = NULL;
  json_bool jname_exists = json_object_object_get_ex(obj, NAME_FIELD, &jname);
  if (!jname_exists) {
    json_object_put(obj);
    return;
  }

  id_ = json_object_get_int64(jid);
  uri_ = common::uri::Uri(json_object_get_string(jurl));
  name_ = json_object_get_string(jname);
  json_object_put(obj);
}

bool Url::IsValid() const {
  return id_ != core::invalid_stream_id && uri_.isValid();
}

common::uri::Uri Url::GetUrl() const {
  return uri_;
}

std::string Url::Name() const {
  return name_;
}

core::stream_id Url::Id() const {
  return id_;
}
