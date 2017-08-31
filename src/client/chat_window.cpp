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

#include "client/player/draw/font.h"

namespace fastotv {
namespace client {

ChatWindow::ChatWindow() : base_class(), hide_button_img_(nullptr), show_button_img_(nullptr) {}

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
  }

  base_class::Draw(render);
}

size_t ChatWindow::GetRowCount() const {
  return 0;
}

void ChatWindow::DrawRow(SDL_Renderer* render, size_t pos, bool is_active_row, const SDL_Rect& row_rect) {
  UNUSED(render);
  UNUSED(pos);
  UNUSED(is_active_row);
  UNUSED(row_rect);
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
  if (!font_) {
    return player::draw::empty_rect;
  }

  int font_height_2line = player::draw::CalcHeightFontPlaceByRowCount(font_, 2);
  SDL_Rect chat_rect = GetRect();
  SDL_Rect show_button_rect = {chat_rect.x + chat_rect.w, chat_rect.h / 2, font_height_2line, font_height_2line};
  return show_button_rect;
}

SDL_Rect ChatWindow::GetShowButtonChatRect() const {
  if (!font_) {
    return player::draw::empty_rect;
  }

  int font_height_2line = player::draw::CalcHeightFontPlaceByRowCount(font_, 2);
  SDL_Rect chat_rect = GetRect();
  SDL_Rect hide_button_rect = {chat_rect.x, chat_rect.h / 2, font_height_2line, font_height_2line};
  return hide_button_rect;
}

}  // namespace client
}  // namespace fastotv
