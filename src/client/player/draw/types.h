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

bool IsEmptyRect(const SDL_Rect& rect);

SDL_Rect GetCenterRect(SDL_Rect rect, int width, int height);

bool IsPointInRect(const SDL_Point& point, const SDL_Rect& rect);

extern const SDL_Rect empty_rect;
extern const SDL_Color white_color;
extern const SDL_Color black_color;
extern const SDL_Color red_color;
extern const SDL_Color green_color;
extern const SDL_Color blue_color;
extern const SDL_Color lightblue_color;
extern const SDL_Color gray_color;

extern const size_t invalid_row_position;

}  // namespace draw
}  // namespace player
}  // namespace client
}  // namespace fastotv
