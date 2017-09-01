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

#include "client/player/gui/widgets/button.h"

#include <common/application/application.h>

namespace fastotv {
namespace client {
namespace player {
namespace gui {

Button::Button() : base_class(), pressed_(false) {}

Button::Button(const SDL_Color& back_ground_color) : base_class(back_ground_color) {}

Button::~Button() {}

bool Button::IsPressed() const {
  return pressed_;
}

void Button::Draw(SDL_Renderer* render) {
  const SDL_Color curr_collor = GetBackGroundColor();
  SDL_Color draw_color = curr_collor;
  if (pressed_) {
    draw_color.r = 0.5 * draw_color.r;
    draw_color.g = 0.5 * draw_color.g;
    draw_color.b = 0.5 * draw_color.b;
  } else if (IsFocused()) {
    draw_color.r = 0.7 * draw_color.r;
    draw_color.g = 0.7 * draw_color.g;
    draw_color.b = 0.7 * draw_color.b;
  }
  SetBackGroundColor(draw_color);
  base_class::Draw(render);
  SetBackGroundColor(curr_collor);
}

void Button::OnFocusChanged(bool focus) {
  pressed_ = false;
  base_class::OnFocusChanged(focus);
}

void Button::OnMouseClicked(Uint8 button, const SDL_Point& position) {
  pressed_ = true;
  base_class::OnMouseClicked(button, position);
}

void Button::OnMouseReleased(Uint8 button, const SDL_Point& position) {
  pressed_ = false;
  base_class::OnMouseReleased(button, position);
}

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
