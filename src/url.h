#pragma once

#include <limits>

#include <common/url.h>

class Url {
 public:
  enum { invalid_id = std::numeric_limits<uint64_t>::max() };
  Url(const std::string& json_data);

  bool IsValid() const;
  common::uri::Uri url() const;
  std::string name() const;

 private:
  uint64_t id_;
  common::uri::Uri uri_;
  std::string name_;
};
