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

#include <SDL2/SDL_ttf.h>

namespace fastotv {
namespace client {
namespace player {
namespace draw {

int CalcHeightFontPlaceByRowCount(const TTF_Font* font, int row);
bool CaclTextSize(const std::string& text, TTF_Font* font, int* width, int* height);
std::string DotText(std::string text, TTF_Font* font, int max_width);
void DrawWrappedTextInRect(SDL_Renderer* render,
                           const std::string& text,
                           TTF_Font* font,
                           SDL_Color text_color,
                           SDL_Rect rect,
                           SDL_Rect* text_rect = NULL);
void DrawCenterTextInRect(SDL_Renderer* render,
                          const std::string& text,
                          TTF_Font* font,
                          SDL_Color text_color,
                          SDL_Rect rect,
                          SDL_Rect* text_rect = NULL);

void DrawImage(SDL_Renderer* render, SDL_Texture* texture, const SDL_Rect& rect);

bool GetTextSize(TTF_Font* font, const std::string& text, int* w, int* h);

}  // namespace draw
}  // namespace player
}  // namespace client
}  // namespace fastotv
