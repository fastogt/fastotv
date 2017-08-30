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

#include <SDL2/SDL_rect.h>

namespace fastotv {
namespace client {
namespace player {
namespace draw {

struct Size {
  Size();
  Size(int width, int height);

  bool IsValid() const;

  int width;
  int height;
};

bool IsValidSize(int width, int height);

SDL_Rect GetCenterRect(SDL_Rect rect, int width, int height);

bool PointInRect(const SDL_Point& point, const SDL_Rect& rect);

}  // namespace draw
}  // namespace player
}  // namespace client
}  // namespace fastotv

namespace common {

std::string ConvertToString(const fastotv::client::player::draw::Size& value);
bool ConvertFromString(const std::string& from, fastotv::client::player::draw::Size* out);

}  // namespace common
