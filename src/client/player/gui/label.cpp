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

#include "client/player/gui/label.h"

#include "client/player/draw/draw.h"

namespace fastotv {
namespace client {
namespace player {
namespace gui {

Label::Label() : text_(), dt_(WRAPPED_TEXT), text_color_(), font_(nullptr) {}

Label::~Label() {}

void Label::SetDrawType(DrawType dt) {
  dt_ = dt;
}

Label::DrawType Label::GetDrawType() const {
  return dt_;
}

void Label::SetText(const std::string& text) {
  text_ = text;
}

std::string Label::GetText() const {
  return text_;
}

void Label::SetTextColor(const SDL_Color& color) {
  text_color_ = color;
}

SDL_Color Label::GetTextColor() const {
  return text_color_;
}

void Label::SetFont(TTF_Font* font) {
  font_ = font;
}

void Label::Draw(SDL_Renderer* render) {
  if (!IsVisible()) {
    return;
  }

  base_class::Draw(render);
  DrawText(render);
}

void Label::DrawText(SDL_Renderer* render) {
  if (dt_ == WRAPPED_TEXT) {
    draw::DrawWrappedTextInRect(render, text_, font_, text_color_, GetRect());
  } else if (dt_ == CENTER_TEXT) {
    draw::DrawCenterTextInRect(render, text_, font_, text_color_, GetRect());
  } else {
    NOTREACHED();
  }
}

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
