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
