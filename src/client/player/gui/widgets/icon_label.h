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

#include "client/player/gui/widgets/label.h"

namespace fastotv {
namespace client {
namespace player {
namespace gui {

class IconLabel : public Label {
 public:
  typedef Label base_class;
  enum { default_space = 1 };

  IconLabel();
  IconLabel(const SDL_Color& back_ground_color);
  virtual ~IconLabel();

  void SetSpace(int space);
  int GetSpace() const;

  void SetIconSize(const draw::Size& icon_size);
  draw::Size GetIconSize() const;

  void SetIconTexture(SDL_Texture* icon_img);
  SDL_Texture* GetIconTexture() const;

  virtual void Draw(SDL_Renderer* render) override;

 private:
  SDL_Texture* icon_img_;
  draw::Size icon_size_;
  int space_betwen_image_and_label_;
};

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
