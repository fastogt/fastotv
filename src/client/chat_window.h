/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

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

#include <player/draw/font.h>
#include <player/gui/widgets/window.h>

#include "commands_info/chat_message.h"

namespace fastoplayer {
namespace gui {
class Button;
class LineEdit;
}  // namespace gui
}  // namespace fastoplayer

namespace fastotv {
namespace client {

class ChatListWindow;

class ChatWindow : public fastoplayer::gui::Window {
 public:
  typedef fastoplayer::gui::Window base_class;
  typedef std::vector<ChatMessage> messages_t;
  enum { login_field_width = 240, space_width = 10, post_button_width = 100 };
  static const SDL_Color text_background_color;

  explicit ChatWindow(const SDL_Color& back_ground_color);
  ~ChatWindow() override;

  std::string GetInputText() const;
  void ClearInputText() const;

  void SetPostMessageEnabled(bool en);

  bool IsActived() const;

  void SetPostClickedCallback(mouse_clicked_callback_t cb);

  void SetTextColor(const SDL_Color& color);

  void SetFont(TTF_Font* font);

  void SetWatchers(size_t watchers);

  void SetMessages(const messages_t& msgs);

  void SetRowHeight(int row_height);

  virtual void Draw(SDL_Renderer* render) override;

 private:
  SDL_Rect GetWatcherRect() const;
  SDL_Rect GetChatRect() const;
  SDL_Rect GetSendButtonRect() const;
  SDL_Rect GetTextInputRect() const;

  size_t watchers_;

  ChatListWindow* chat_window_;
  fastoplayer::gui::Button* send_message_button_;
  fastoplayer::gui::LineEdit* text_input_box_;

  TTF_Font* font_;
  SDL_Color text_color_;
  bool post_message_enabled_;
};

}  // namespace client
}  // namespace fastotv
