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

#include "client/player/gui/widgets/list_box.h"

namespace fastotv {
namespace client {

class ChatWindow : public player::gui::IListBox {
 public:
  typedef player::gui::IListBox base_class;
  ChatWindow(const SDL_Color& back_ground_color);

  void SetHideButtonImage(SDL_Texture* img);

  void SetShowButtonImage(SDL_Texture* img);

  virtual void Draw(SDL_Renderer* render) override;

  virtual size_t GetRowCount() const override;

 protected:
  virtual void DrawRow(SDL_Renderer* render, size_t pos, bool is_active_row, const SDL_Rect& row_rect) override;

  virtual void HandleMousePressEvent(player::gui::events::MousePressEvent* event) override;

 private:
  bool IsHideButtonChatRect(const SDL_Point& point) const;
  bool IsShowButtonChatRect(const SDL_Point& point) const;

  SDL_Rect GetHideButtonChatRect() const;
  SDL_Rect GetShowButtonChatRect() const;

  SDL_Texture* hide_button_img_;
  SDL_Texture* show_button_img_;
};

}  // namespace client
}  // namespace fastotv
