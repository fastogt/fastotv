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

#include "client/chat_list_window.h"

#include <string>

namespace fastotv {
namespace client {

ChatListWindow::ChatListWindow(const SDL_Color& back_ground_color) : base_class(back_ground_color), msgs_() {}

void ChatListWindow::SetMessages(const messages_t& msgs) {
  msgs_ = msgs;
}

ChatListWindow::messages_t ChatListWindow::GetMessages() const {
  return msgs_;
}

size_t ChatListWindow::GetRowCount() const {
  return msgs_.size();
}

void ChatListWindow::DrawRow(SDL_Renderer* render, size_t pos, bool active, bool hover, const SDL_Rect& row_rect) {
  UNUSED(active);

  ChatMessage msg = msgs_[pos];
  std::string login = msg.GetLogin();
  SDL_Rect login_rect = {row_rect.x, row_rect.y, login_field_width, row_rect.h};
  std::string login_text_doted = fastoplayer::draw::DotText(login, GetFont(), login_rect.w);
  base_class::DrawText(render, login_text_doted + ":", login_rect, DrawType::CENTER_TEXT);

  std::string mess_text = msg.GetMessage();
  SDL_Rect text_rect = {row_rect.x + login_field_width + space_width, row_rect.y,
                        row_rect.w - login_field_width - space_width, row_rect.h};
  std::string mess_text_doted = fastoplayer::draw::DotText(mess_text, GetFont(), text_rect.w);
  base_class::DrawText(render, mess_text_doted, text_rect, DrawType::WRAPPED_TEXT);
}

}  // namespace client
}  // namespace fastotv
