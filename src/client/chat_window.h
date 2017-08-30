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

#include <SDL2/SDL_ttf.h>

#include "client/player/gui/window.h"
#include "client/player/draw/surface_saver.h"

namespace fastotv {
namespace client {

class ChatWindow : public player::gui::Window {
 public:
  typedef player::gui::Window base_class;
  ChatWindow();

  void SetHideButtonTexture(player::draw::SurfaceSaver* sv);

  void SetShowButtonTexture(player::draw::SurfaceSaver* sv);

  void SetFont(TTF_Font* font);

  virtual void Draw(SDL_Renderer* render, const SDL_Rect& rect, const SDL_Color& back_ground_color) override;

 protected:
  virtual void HandleMousePressEvent(player::core::events::MousePressEvent* event) override;

 private:
  bool IsHideButtonChatRect(const SDL_Point& point) const;
  bool IsShowButtonChatRect(const SDL_Point& point) const;
  SDL_Rect GetHideButtonChatRect() const;
  SDL_Rect GetShowButtonChatRect() const;

  player::draw::SurfaceSaver* hide_button_texture_;
  player::draw::SurfaceSaver* show_button_texture_;
  TTF_Font* font_;
  SDL_Rect rect_;
};

}  // namespace client
}  // namespace fastotv
