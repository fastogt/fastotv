#pragma once

#include <limits>

#include <common/url.h>

#include "core/types.h"

class Url {
 public:
  explicit Url(const std::string& json_data);

  bool IsValid() const;
  common::uri::Uri GetUrl() const;
  std::string Name() const;
  core::stream_id Id() const;

 private:
  core::stream_id id_;
  common::uri::Uri uri_;
  std::string name_;
};
