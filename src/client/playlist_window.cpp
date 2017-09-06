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

#include "client/player/draw/draw.h"
#include "client/player/draw/surface_saver.h"

namespace fastotv {
namespace client {

PlaylistWindow::PlaylistWindow(const SDL_Color& back_ground_color)
    : base_class(back_ground_color),
      play_list_(nullptr),
      current_position_in_playlist_(player::draw::invalid_row_position),
      select_cur_color_() {}

PlaylistWindow::~PlaylistWindow() {}

void PlaylistWindow::SetPlaylist(const playlist_t* pl) {
  play_list_ = pl;
}

const PlaylistWindow::playlist_t* PlaylistWindow::GetPlaylist() const {
  return play_list_;
}

size_t PlaylistWindow::GetRowCount() const {
  if (!play_list_) {
    return 0;
  }
  return play_list_->size();
}

void PlaylistWindow::SetCurrentPositionInPlaylist(size_t pos) {
  current_position_in_playlist_ = pos;
}

void PlaylistWindow::SetCurrentPositionSelectionColor(const SDL_Color& sel) {
  select_cur_color_ = sel;
}

SDL_Color PlaylistWindow::GetCurrentPositionSelectionColor() const {
  return select_cur_color_;
}

void PlaylistWindow::DrawRow(SDL_Renderer* render, size_t pos, bool is_active_row, const SDL_Rect& row_rect) {
  UNUSED(is_active_row);

  if (!play_list_) {
    return;
  }

  SDL_Rect number_rect = {row_rect.x, row_rect.y, channel_number_width, row_rect.h};
  std::string number_str = common::ConvertToString(pos + 1);
  DrawText(render, number_str, number_rect, PlaylistWindow::CENTER_TEXT);

  ChannelDescription descr = play_list_->operator[](pos).GetChannelDescription();
  channel_icon_t icon = descr.icon;
  int shift = channel_number_width;
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

  if (pos == current_position_in_playlist_) {
    player::draw::FillRectColor(render, row_rect, select_cur_color_);
  }
}

}  // namespace client
}  // namespace fastotv
