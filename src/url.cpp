#include "url.h"

#include "third-party/json-c/json-c/json.h"  // for json_object_...

#define ID_FIELD "id"
#define URL_FIELD "url"
#define NAME_FIELD "name"

namespace fasto {
namespace fastotv {
Url::Url() : id_(invalid_stream_id), uri_(), name_() {}

Url::Url(stream_id id, const common::uri::Uri& uri, const std::string& name)
    : id_(id), uri_(uri), name_(name) {}

bool Url::IsValid() const {
  return id_ != invalid_stream_id && uri_.isValid();
}

common::uri::Uri Url::GetUrl() const {
  return uri_;
}

std::string Url::Name() const {
  return name_;
}

stream_id Url::Id() const {
  return id_;
}

struct json_object* Url::MakeJobject(const Url& url) {
  json_object* obj = json_object_new_object();
  json_object_object_add(obj, ID_FIELD, json_object_new_int64(url.Id()));
  common::uri::Uri uri = url.GetUrl();
  const std::string url_str = uri.url();
  json_object_object_add(obj, URL_FIELD, json_object_new_string(url_str.c_str()));
  const std::string name_str = url.Name();
  json_object_object_add(obj, NAME_FIELD, json_object_new_string(name_str.c_str()));
  return obj;
}

fasto::fastotv::Url Url::MakeClass(struct json_object* obj) {
  json_object* jid = NULL;
  json_bool jid_exists = json_object_object_get_ex(obj, ID_FIELD, &jid);
  if (!jid_exists) {
    return fasto::fastotv::Url();
  }

  json_object* jurl = NULL;
  json_bool jurls_exists = json_object_object_get_ex(obj, URL_FIELD, &jurl);
  if (!jurls_exists) {
    return fasto::fastotv::Url();
  }

  json_object* jname = NULL;
  json_bool jname_exists = json_object_object_get_ex(obj, NAME_FIELD, &jname);
  if (!jname_exists) {
    return fasto::fastotv::Url();
  }

  fasto::fastotv::Url url(json_object_get_int64(jid),
                          common::uri::Uri(json_object_get_string(jurl)),
                          json_object_get_string(jname));
  return url;
}
}
}
