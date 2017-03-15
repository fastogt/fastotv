#pragma once

#include <limits>

#include <common/url.h>

#include "common_types.h"

struct json_object;

namespace fasto {
namespace fastotv {

class Url {
 public:
  Url();
  Url(stream_id id, const common::uri::Uri& uri, const std::string& name);

  bool IsValid() const;
  common::uri::Uri GetUrl() const;
  std::string Name() const;
  stream_id Id() const;

  static json_object* MakeJobject(const Url& url);  // allocate json_object
  static Url MakeClass(json_object* obj);           // pass valid json obj

 private:
  stream_id id_;
  common::uri::Uri uri_;
  std::string name_;
};
}
}
