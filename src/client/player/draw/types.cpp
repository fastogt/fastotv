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

const size_t invalid_row_position = static_cast<size_t>(-1);

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
