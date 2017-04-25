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

struct Point {
  Point() : x(0), y(0) {}
  Point(int x, int y) : x(x), y(y) {}

  int x;
  int y;
};

struct Size {
  Size() : width(0), height(0) {}
  Size(int width, int height) : width(width), height(height) {}

  bool IsValid() { return width != 0 && height != 0; }

  int width;
  int height;
};

struct Rect {
  Rect(int x, int y, int width, int height) : x(x), y(y), w(width), h(height) {}
  int x, y;
  int w, h;
};
}
}

namespace common {
std::string ConvertToString(const fasto::fastotv::Point& value);
bool ConvertFromString(const std::string& from, fasto::fastotv::Point* out);

std::string ConvertToString(const fasto::fastotv::Size& value);
bool ConvertFromString(const std::string& from, fasto::fastotv::Size* out);
}
