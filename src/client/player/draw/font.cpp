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

#include "client/player/draw/font.h"

#include <math.h>

#include <common/utils.h>

#include "client/player/draw/draw.h"

namespace fastotv {
namespace client {
namespace player {
namespace draw {

int CalcHeightFontPlaceByRowCount(const TTF_Font* font, int row) {
  if (!font) {
    return 0;
  }

  int font_height = TTF_FontLineSkip(font);
  return 2 << static_cast<int>(log2(font_height * row));
}

bool CaclTextSize(const std::string& text, TTF_Font* font, int* width, int* height) {
  if (text.empty() || !font || !width || !height) {
    return false;
  }

  const char* text_ptr = text.c_str();
  int res = TTF_SizeText(font, text_ptr, width, height);
  return res == 0;
}

std::string DotText(std::string text, TTF_Font* font, int max_width) {
  int needed_width, needed_height;
  if (CaclTextSize(text, font, &needed_width, &needed_height) && needed_width > max_width) {
    int char_width = needed_width / text.size();
    int diff = max_width / char_width;
    if (diff - 3 > 0) {
      text = text.substr(0, diff - 3) + "...";
    }
  }

  return text;
}

void DrawWrappedTextInRect(SDL_Renderer* render,
                           const std::string& text,
                           TTF_Font* font,
                           SDL_Color text_color,
                           SDL_Rect rect,
                           SDL_Rect* text_rect) {
  if (!render || !font || text.empty()) {
    return;
  }

  const char* text_ptr = text.c_str();
  SDL_Surface* text_surf = TTF_RenderUTF8_Blended_Wrapped(font, text_ptr, text_color, rect.w);
  SDL_Texture* texture = SDL_CreateTextureFromSurface(render, text_surf);
  SDL_Rect dst = rect;
  dst.w = text_surf->w;
  SDL_RenderCopy(render, texture, NULL, &dst);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(text_surf);
  if (text_rect) {
    *text_rect = dst;
  }
}

void DrawCenterTextInRect(SDL_Renderer* render,
                          const std::string& text,
                          TTF_Font* font,
                          SDL_Color text_color,
                          SDL_Rect rect,
                          SDL_Rect* text_rect) {
  if (!render || !font || text.empty()) {
    return;
  }

  const char* text_ptr = text.c_str();
  SDL_Surface* text_surf = TTF_RenderUTF8_Blended(font, text_ptr, text_color);
  SDL_Rect dst = GetCenterRect(rect, text_surf->w, text_surf->h);
  SDL_Texture* texture = SDL_CreateTextureFromSurface(render, text_surf);
  SDL_RenderCopy(render, texture, NULL, &dst);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(text_surf);

  if (text_rect) {
    *text_rect = dst;
  }
}

void DrawImage(SDL_Renderer* render, SDL_Texture* texture, const SDL_Rect& rect) {
  if (!render || !texture) {
    return;
  }

  SDL_RenderCopy(render, texture, NULL, &rect);
}

bool GetTextSize(TTF_Font* font, const std::string& text, int* w, int* h) {
  if (!font || text.empty()) {
    return false;
  }

  const char* text_ptr = text.c_str();
  return TTF_SizeUTF8(font, text_ptr, w, h) == 0;
}

}  // namespace draw
}  // namespace player
}  // namespace client
}  // namespace fastotv
