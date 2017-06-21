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

#include <string>

namespace fasto {
namespace fastotv {
namespace client {

enum BandwidthHostType { UNKNOWN_SERVER, MAIN_SERVER, CHANNEL_SERVER };

struct Point {
  Point();
  Point(int x, int y);

  int x;
  int y;
};

struct Rational {
  int num;  ///< Numerator
  int den;  ///< Denominator
};

struct Rect {
  Rect(int x, int y, int width, int height);
  int x, y;
  int width, height;
};
}  // namespace client
}  // namespace fastotv
}  // namespace fasto

namespace common {
std::string ConvertToString(const fasto::fastotv::client::Point& value);
bool ConvertFromString(const std::string& from, fasto::fastotv::client::Point* out);
}  // namespace common
