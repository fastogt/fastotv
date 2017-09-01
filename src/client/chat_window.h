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

#include "chat_message.h"

namespace fastotv {
namespace client {

class ChatWindow : public player::gui::IListBox {
 public:
  typedef player::gui::IListBox base_class;
  typedef std::vector<ChatMessage> messages_t;
  enum { login_field_width = 240, space_width = 10 };
  ChatWindow(const SDL_Color& back_ground_color);

  void SetWatchers(size_t watchers);
  size_t GetWatchers() const;

  void SetMessages(const messages_t& msgs);
  messages_t GetMessages() const;

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
  SDL_Rect GetWatcherRect() const;

  SDL_Texture* hide_button_img_;
  SDL_Texture* show_button_img_;
  messages_t msgs_;
  size_t watchers_;
};

}  // namespace client
}  // namespace fastotv
