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

#include "client/chat_window.h"

#include <common/application/application.h>
#include <common/convert2string.h>

#include "client/player/draw/font.h"
#include "client/player/draw/draw.h"

namespace fastotv {
namespace client {

ChatWindow::ChatWindow(const SDL_Color& back_ground_color)
    : base_class(back_ground_color), hide_button_img_(nullptr), show_button_img_(nullptr), msgs_(), watchers_(0) {}

void ChatWindow::SetWatchers(size_t watchers) {
  watchers_ = watchers;
}

size_t ChatWindow::GetWatchers() const {
  return watchers_;
}

void ChatWindow::SetMessages(const messages_t& msgs) {
  msgs_ = msgs;
}

ChatWindow::messages_t ChatWindow::GetMessages() const {
  return msgs_;
}

void ChatWindow::SetHideButtonImage(SDL_Texture* img) {
  hide_button_img_ = img;
}

void ChatWindow::SetShowButtonImage(SDL_Texture* img) {
  show_button_img_ = img;
}

void ChatWindow::Draw(SDL_Renderer* render) {
  if (!IsCanDraw()) {
    base_class::Draw(render);
    return;
  }

  if (fApp->IsCursorVisible()) {
    if (!IsVisible()) {
      if (show_button_img_) {
        SDL_Rect show_button_rect = GetShowButtonChatRect();
        SDL_RenderCopy(render, show_button_img_, NULL, &show_button_rect);
      }
      return;
    }

    if (hide_button_img_) {
      SDL_Rect hide_button_rect = GetHideButtonChatRect();
      SDL_RenderCopy(render, hide_button_img_, NULL, &hide_button_rect);
    }

    SDL_Rect watchers_rect = GetWatcherRect();
    std::string watchers_str = common::ConvertToString(watchers_);
    player::draw::FillRectColor(render, watchers_rect, player::draw::red_color);
    base_class::DrawText(render, watchers_str.c_str(), watchers_rect, DrawType::CENTER_TEXT);
  }

  base_class::Draw(render);
}

size_t ChatWindow::GetRowCount() const {
  return msgs_.size();
}

void ChatWindow::DrawRow(SDL_Renderer* render, size_t pos, bool is_active_row, const SDL_Rect& row_rect) {
  UNUSED(is_active_row);

  ChatMessage msg = msgs_[pos];
  std::string login = msg.GetLogin();
  SDL_Rect login_rect = {row_rect.x, row_rect.y, login_field_width, row_rect.h};
  std::string login_text_doted = player::draw::DotText(login, GetFont(), login_rect.w);
  base_class::DrawText(render, login_text_doted + ":", login_rect, DrawType::CENTER_TEXT);

  std::string mess_text = msg.GetMessage();
  SDL_Rect text_rect = {row_rect.x + login_field_width + space_width, row_rect.y,
                        row_rect.w - login_field_width - space_width, row_rect.h};
  std::string mess_text_doted = player::draw::DotText(mess_text, GetFont(), text_rect.w);
  base_class::DrawText(render, mess_text_doted, text_rect, DrawType::WRAPPED_TEXT);
}

void ChatWindow::HandleMousePressEvent(player::gui::events::MousePressEvent* event) {
  player::gui::events::MousePressInfo inf = event->GetInfo();
  SDL_MouseButtonEvent sinfo = inf.mevent;
  if (sinfo.button == SDL_BUTTON_LEFT) {
    SDL_Point point = inf.GetMousePoint();
    if (IsVisible()) {
      if (IsHideButtonChatRect(point)) {
        SetVisible(false);
      }
    } else {
      if (IsShowButtonChatRect(point)) {
        SetVisible(true);
      }
    }
  }

  base_class::HandleMousePressEvent(event);
}

bool ChatWindow::IsHideButtonChatRect(const SDL_Point& point) const {
  const SDL_Rect hide_button_rect = GetHideButtonChatRect();
  return player::draw::IsPointInRect(point, hide_button_rect);
}

bool ChatWindow::IsShowButtonChatRect(const SDL_Point& point) const {
  const SDL_Rect show_button_rect = GetShowButtonChatRect();
  return player::draw::IsPointInRect(point, show_button_rect);
}

SDL_Rect ChatWindow::GetHideButtonChatRect() const {
  TTF_Font* font = GetFont();
  if (!font) {
    return player::draw::empty_rect;
  }

  int font_height_2line = player::draw::CalcHeightFontPlaceByRowCount(font, 2);
  SDL_Rect chat_rect = GetRect();
  SDL_Rect show_button_rect = {chat_rect.w / 2, chat_rect.y - font_height_2line, font_height_2line, font_height_2line};
  return show_button_rect;
}

SDL_Rect ChatWindow::GetShowButtonChatRect() const {
  TTF_Font* font = GetFont();
  if (!font) {
    return player::draw::empty_rect;
  }

  int font_height_2line = player::draw::CalcHeightFontPlaceByRowCount(font, 2);
  SDL_Rect chat_rect = GetRect();
  SDL_Rect hide_button_rect = {chat_rect.w / 2, chat_rect.y + chat_rect.h - font_height_2line, font_height_2line,
                               font_height_2line};
  return hide_button_rect;
}

SDL_Rect ChatWindow::GetWatcherRect() const {
  TTF_Font* font = GetFont();
  if (!font) {
    return player::draw::empty_rect;
  }

  int font_height_2line = player::draw::CalcHeightFontPlaceByRowCount(font, 2);
  SDL_Rect chat_rect = GetRect();
  SDL_Rect hide_button_rect = {chat_rect.w - font_height_2line, chat_rect.y - font_height_2line, font_height_2line,
                               font_height_2line};
  return hide_button_rect;
}

}  // namespace client
}  // namespace fastotv
