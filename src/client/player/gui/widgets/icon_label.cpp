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

#include "client/player/gui/widgets/icon_label.h"

#include <common/application/application.h>

namespace fastotv {
namespace client {
namespace player {
namespace gui {

IconLabel::IconLabel() : base_class(), icon_img_(nullptr), icon_size_(), space_betwen_image_and_label_(default_space) {}

IconLabel::IconLabel(const SDL_Color& back_ground_color)
    : base_class(back_ground_color), icon_img_(nullptr), icon_size_(), space_betwen_image_and_label_(default_space) {}

IconLabel::~IconLabel() {}

void IconLabel::SetSpace(int space) {
  space_betwen_image_and_label_ = space;
}

int IconLabel::GetSpace() const {
  return space_betwen_image_and_label_;
}

void IconLabel::Draw(SDL_Renderer* render) {
  if (!IsVisible()) {
    return;
  }

  if (!icon_img_) {
    base_class::Draw(render);
    return;
  }

  Window::Draw(render);
  SDL_Rect area_rect = GetRect();
  SDL_Rect icon_rect = {area_rect.x, area_rect.y, icon_size_.height, icon_size_.width};
  SDL_RenderCopy(render, icon_img_, NULL, &icon_rect);
  int shift = icon_size_.width + space_betwen_image_and_label_;

  SDL_Rect text_rect = {area_rect.x + shift, area_rect.y, area_rect.w - shift, area_rect.h};
  base_class::DrawText(render, text_, text_rect, GetDrawType());
}

void IconLabel::SetIconSize(const draw::Size& icon_size) {
  icon_size_ = icon_size;
}

draw::Size IconLabel::GetIconSize() const {
  return icon_size_;
}

void IconLabel::SetIconTexture(SDL_Texture* icon_img) {
  icon_img_ = icon_img;
}

SDL_Texture* IconLabel::GetIconTexture() const {
  return icon_img_;
}

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
