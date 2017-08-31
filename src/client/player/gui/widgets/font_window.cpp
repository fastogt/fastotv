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

#include "client/player/gui/widgets/font_window.h"

#include "client/player/draw/draw.h"

namespace fastotv {
namespace client {
namespace player {
namespace gui {

FontWindow::FontWindow() : base_class(), draw_type_(WRAPPED_TEXT), text_color_(), font_(nullptr) {}

FontWindow::FontWindow(const SDL_Color& back_ground_color)
    : base_class(back_ground_color), draw_type_(WRAPPED_TEXT), text_color_(), font_(nullptr) {}

FontWindow::~FontWindow() {}

void FontWindow::SetDrawType(DrawType dt) {
  draw_type_ = dt;
}

FontWindow::DrawType FontWindow::GetDrawType() const {
  return draw_type_;
}

void FontWindow::SetTextColor(const SDL_Color& color) {
  text_color_ = color;
}

SDL_Color FontWindow::GetTextColor() const {
  return text_color_;
}

void FontWindow::SetFont(TTF_Font* font) {
  font_ = font;
}

TTF_Font* FontWindow::GetFont() const {
  return font_;
}

void FontWindow::DrawText(SDL_Renderer* render, const std::string& text, const SDL_Rect& rect, DrawType dt) {
  if (dt == WRAPPED_TEXT) {
    draw::DrawWrappedTextInRect(render, text, font_, text_color_, rect);
  } else if (dt == CENTER_TEXT) {
    draw::DrawCenterTextInRect(render, text, font_, text_color_, rect);
  } else {
    NOTREACHED();
  }
}

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
