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

#include "client/playlist_window.h"

#include <common/application/application.h>

#include <common/convert2string.h>

#include "client/player/draw/surface_saver.h"

namespace fastotv {
namespace client {

PlaylistWindow::PlaylistWindow() : base_class(), hide_button_img_(nullptr), show_button_img_(nullptr), play_list_() {}

PlaylistWindow::~PlaylistWindow() {}

void PlaylistWindow::SetPlaylist(const playlist_t& pl) {
  play_list_ = pl;
}

PlaylistWindow::playlist_t PlaylistWindow::GetPlaylist() const {
  return play_list_;
}

void PlaylistWindow::SetHideButtonImage(SDL_Texture* img) {
  hide_button_img_ = img;
}

void PlaylistWindow::SetShowButtonImage(SDL_Texture* img) {
  show_button_img_ = img;
}

size_t PlaylistWindow::GetRowCount() const {
  return play_list_.size();
}

void PlaylistWindow::Draw(SDL_Renderer* render) {
  if (!IsVisible()) {
    if (fApp->IsCursorVisible()) {
      if (show_button_img_) {
        SDL_Rect show_button_rect = GetShowButtonProgramsListRect();
        SDL_RenderCopy(render, show_button_img_, NULL, &show_button_rect);
      }
    }
    return;
  }

  if (hide_button_img_) {
    SDL_Rect hide_button_rect = GetHideButtonProgramsListRect();
    SDL_RenderCopy(render, hide_button_img_, NULL, &hide_button_rect);
  }

  base_class::Draw(render);
}

void PlaylistWindow::DrawRow(SDL_Renderer* render, size_t pos, bool is_active_row, const SDL_Rect& row_rect) {
  int shift = 0;
  SDL_Rect number_rect = {row_rect.x, row_rect.y, keypad_width, row_rect.h};
  std::string number_str = common::ConvertToString(pos + 1);
  DrawText(render, number_str, number_rect, PlaylistWindow::CENTER_TEXT);

  auto descr = play_list_[pos];
  channel_icon_t icon = descr.icon;
  shift = keypad_width;  // in any case shift should be
  if (icon) {
    SDL_Texture* img = icon->GetTexture(render);
    if (img) {
      SDL_Rect icon_rect = {row_rect.x + shift, row_rect.y, row_rect.h, row_rect.h};
      SDL_RenderCopy(render, img, NULL, &icon_rect);
    }
  }
  shift += row_rect.h + space_width;  // in any case shift should be

  int text_width = row_rect.w - shift;
  std::string title_line = player::draw::DotText(common::MemSPrintf("Title: %s", descr.title), GetFont(), text_width);
  std::string description_line =
      player::draw::DotText(common::MemSPrintf("Description: %s", descr.description), GetFont(), text_width);

  std::string line_text = common::MemSPrintf(
      "%s\n"
      "%s",
      title_line, description_line);
  SDL_Rect text_rect = {row_rect.x + shift, row_rect.y, text_width, row_rect.h};
  DrawText(render, line_text, text_rect, GetDrawType());
}

void PlaylistWindow::HandleMousePressEvent(player::gui::events::MousePressEvent* event) {
  player::gui::events::MousePressInfo inf = event->GetInfo();
  SDL_MouseButtonEvent sinfo = inf.mevent;
  if (sinfo.button == SDL_BUTTON_LEFT) {
    SDL_Point point = inf.GetMousePoint();
    if (IsVisible()) {
      if (IsHideButtonProgramsListRect(point)) {
        SetVisible(false);
      }
    } else {
      if (IsShowButtonProgramsListRect(point)) {
        SetVisible(true);
      }
    }
  }

  base_class::HandleMousePressEvent(event);
}

bool PlaylistWindow::IsHideButtonProgramsListRect(const SDL_Point& point) const {
  const SDL_Rect hide_button_rect = GetHideButtonProgramsListRect();
  return player::draw::IsPointInRect(point, hide_button_rect);
}

bool PlaylistWindow::IsShowButtonProgramsListRect(const SDL_Point& point) const {
  const SDL_Rect show_button_rect = GetShowButtonProgramsListRect();
  return player::draw::IsPointInRect(point, show_button_rect);
}

SDL_Rect PlaylistWindow::GetHideButtonProgramsListRect() const {
  TTF_Font* font = GetFont();
  if (!font) {
    return SDL_Rect();
  }

  int font_height_2line = player::draw::CalcHeightFontPlaceByRowCount(font, 2);
  SDL_Rect prog_rect = GetRect();
  SDL_Rect hide_button_rect = {prog_rect.x - font_height_2line, prog_rect.h / 2, font_height_2line, font_height_2line};
  return hide_button_rect;
}

SDL_Rect PlaylistWindow::GetShowButtonProgramsListRect() const {
  TTF_Font* font = GetFont();
  if (!font) {
    return SDL_Rect();
  }

  int font_height_2line = player::draw::CalcHeightFontPlaceByRowCount(font, 2);
  SDL_Rect prog_rect = GetRect();
  SDL_Rect show_button_rect = {prog_rect.x + prog_rect.w - font_height_2line, prog_rect.h / 2, font_height_2line,
                               font_height_2line};
  return show_button_rect;
}

}  // namespace client
}  // namespace fastotv
