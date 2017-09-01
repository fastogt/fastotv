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

#include "client/player/draw/font.h"

#include "client/player/gui/widgets/window.h"

#include "chat_message.h"

namespace fastotv {
namespace client {
namespace player {
namespace gui {
class Button;
}
}  // namespace player

class ChatListWindow;

class ChatWindow : public player::gui::Window {
 public:
  typedef player::gui::Window base_class;
  typedef std::vector<ChatMessage> messages_t;
  enum { login_field_width = 240, space_width = 10 };
  ChatWindow(const SDL_Color& back_ground_color);
  ~ChatWindow();

  void SetPostClickedCallback(mouse_clicked_callback_t cb);

  void SetTextColor(const SDL_Color& color);

  void SetFont(TTF_Font* font);

  void SetWatchers(size_t watchers);

  void SetHideButtonImage(SDL_Texture* img);

  void SetShowButtonImage(SDL_Texture* img);

  void SetMessages(const messages_t& msgs);

  void SetRowHeight(int row_height);

  virtual void Draw(SDL_Renderer* render) override;

 protected:
  virtual void HandleMousePressEvent(player::gui::events::MousePressEvent* event) override;

 private:
  bool IsHideButtonChatRect(const SDL_Point& point) const;
  bool IsShowButtonChatRect(const SDL_Point& point) const;

  SDL_Rect GetHideButtonChatRect() const;
  SDL_Rect GetShowButtonChatRect() const;
  SDL_Rect GetWatcherRect() const;
  SDL_Rect GetSendButtonRect() const;
  SDL_Rect GetChatRect() const;

  SDL_Texture* hide_button_img_;
  SDL_Texture* show_button_img_;
  size_t watchers_;

  ChatListWindow* chat_window_;
  player::gui::Button* send_message_button_;

  TTF_Font* font_;
  SDL_Color text_color_;
};

}  // namespace client
}  // namespace fastotv
