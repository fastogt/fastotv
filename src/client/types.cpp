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

#include "client/types.h"

#include <common/convert2string.h>
#include <common/sprintf.h>

namespace fasto {
namespace fastotv {
namespace client {

Point::Point() : x(0), y(0) {}

Point::Point(int x, int y) : x(x), y(y) {}

Rect::Rect(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) {}

}  // namespace client
}  // namespace fastotv
}  // namespace fasto

namespace common {

std::string ConvertToString(const fasto::fastotv::client::Point& value) {
  return MemSPrintf("%d:%d", value.x, value.y);
}

bool ConvertFromString(const std::string& from, fasto::fastotv::client::Point* out) {
  if (!out) {
    return false;
  }

  fasto::fastotv::client::Point res;
  size_t del = from.find_first_of(':');
  if (del != std::string::npos) {
    int lx;
    if (!ConvertFromString(from.substr(0, del), &lx)) {
      return false;
    }
    res.x = lx;

    int ly;
    if (!ConvertFromString(from.substr(del + 1), &ly)) {
      return false;
    }
    res.y = ly;
  }

  *out = res;
  return true;
}
}  // namespace common
