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

#include "client/player/gui/widgets/label.h"

namespace fastotv {
namespace client {
namespace player {
namespace gui {

Label::Label() : base_class(), text_() {}

Label::Label(const SDL_Color& back_ground_color) : base_class(back_ground_color), text_() {}

Label::~Label() {}

void Label::SetText(const std::string& text) {
  text_ = text;
}

std::string Label::GetText() const {
  return text_;
}

void Label::ClearText() {
  text_.clear();
}

void Label::Draw(SDL_Renderer* render) {
  DrawLabel(render, NULL);
}

void Label::DrawLabel(SDL_Renderer* render, SDL_Rect* text_rect) {
  if (!IsVisible() || !IsCanDraw()) {
    return;
  }

  base_class::Draw(render);
  base_class::DrawText(render, text_, GetRect(), GetDrawType(), text_rect);
}

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
