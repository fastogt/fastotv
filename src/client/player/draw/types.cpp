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

#include "client/player/draw/types.h"

#include <common/convert2string.h>
#include <common/sprintf.h>

namespace fastotv {
namespace client {
namespace player {
namespace draw {

const SDL_Rect empty_rect = {0, 0, 0, 0};
const SDL_Color white_color = {255, 255, 255, 255};
const SDL_Color black_color = {0, 0, 0, 255};
const SDL_Color gray_color = {200, 200, 200, 255};
const SDL_Color green_color = {0, 150, 0, 255};
const SDL_Color red_color = {255, 0, 0, 255};
const SDL_Color blue_color = {0, 0, 255, 255};
const SDL_Color lightblue_color = {200, 225, 255, 255};

const Size invalid_size = {0, 0};
const size_t invalid_row_position = static_cast<size_t>(-1);

Size::Size() : width(invalid_size.width), height(invalid_size.height) {}

Size::Size(int width, int height) : width(width), height(height) {}

bool Size::IsValid() const {
  return IsValidSize(width, height);
}

bool Size::Equals(const Size& sz) const {
  return width == sz.width && height == sz.height;
}

SDL_Rect GetCenterRect(SDL_Rect rect, int width, int height) {
  int calc_width = rect.w;
  int calc_height = rect.h;
  int calc_x = rect.x;
  int calc_y = rect.y;

  if (rect.w >= width) {
    calc_x = rect.x + rect.w / 2 - width / 2;
    calc_width = width;
  }
  if (rect.h >= height) {
    calc_y = rect.y + rect.h / 2 - height / 2;
    calc_height = height;
  }

  return {calc_x, calc_y, calc_width, calc_height};
}

bool IsPointInRect(const SDL_Point& point, const SDL_Rect& rect) {
  return SDL_PointInRect(&point, &rect);
}

bool IsValidSize(int width, int height) {
  return width > 0 && height > 0;
}

bool IsEmptyRect(const SDL_Rect& rect) {
  return SDL_RectEmpty(&rect);
}

}  // namespace draw
}  // namespace player
}  // namespace client
}  // namespace fastotv

namespace common {

std::string ConvertToString(const fastotv::client::player::draw::Size& value) {
  return MemSPrintf("%dx%d", value.width, value.height);
}

bool ConvertFromString(const std::string& from, fastotv::client::player::draw::Size* out) {
  if (!out) {
    return false;
  }

  fastotv::client::player::draw::Size res;
  size_t del = from.find_first_of('x');
  if (del != std::string::npos) {
    int lwidth;
    if (!ConvertFromString(from.substr(0, del), &lwidth)) {
      return false;
    }
    res.width = lwidth;

    int lheight;
    if (!ConvertFromString(from.substr(del + 1), &lheight)) {
      return false;
    }
    res.height = lheight;
  }

  *out = res;
  return true;
}

}  // namespace common
