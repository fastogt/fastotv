/*  Copyright (C) 2014-2020 FastoGT. All right reserved.

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

#include "client/vod/vods_window.h"

#include <string>

#include <common/application/application.h>

#include <common/convert2string.h>
#include <common/sprintf.h>

#include <player/draw/draw.h>
#include <player/draw/surface_saver.h>

namespace fastotv {
namespace client {

VodsWindow::VodsWindow(const SDL_Color& back_ground_color, Window* parent)
    : base_class(back_ground_color, parent), play_list_(nullptr) {}

VodsWindow::~VodsWindow() {}

void VodsWindow::SetPlaylist(const playlist_t* pl) {
  play_list_ = pl;
}

const VodsWindow::playlist_t* VodsWindow::GetPlaylist() const {
  return play_list_;
}

size_t VodsWindow::GetRowCount() const {
  if (!play_list_) {
    return 0;
  }
  return play_list_->size();
}

void VodsWindow::DrawRow(SDL_Renderer* render, size_t pos, bool active, bool hover, const SDL_Rect& row_rect) {
  UNUSED(active);
  UNUSED(hover);

  if (!play_list_) {
    return;
  }

  SDL_Rect number_rect = {row_rect.x, row_rect.y, channel_number_width, row_rect.h};
  std::string number_str = common::ConvertToString(pos + 1);
  DrawText(render, number_str, number_rect, VodsWindow::CENTER_TEXT);

  VodDescription descr = play_list_->operator[](pos).GetChannelDescription();
  channel_icon_t icon = descr.icon;
  int shift = channel_number_width;
  if (icon) {
    SDL_Texture* img = icon->GetTexture(render);
    if (img) {
      SDL_Rect icon_rect = {row_rect.x + shift, row_rect.y, row_rect.h, row_rect.h};
      SDL_RenderCopy(render, img, nullptr, &icon_rect);
    }
  }
  shift += row_rect.h + space_width;  // in any case shift should be

  int text_width = row_rect.w - shift;
  std::string title_line =
      fastoplayer::draw::DotText(common::MemSPrintf("Title: %s", descr.title), GetFont(), text_width);
  std::string description_line =
      fastoplayer::draw::DotText(common::MemSPrintf("Description: %s", descr.description), GetFont(), text_width);

  std::string line_text = common::MemSPrintf(
      "%s\n"
      "%s",
      title_line, description_line);
  SDL_Rect text_rect = {row_rect.x + shift, row_rect.y, text_width, row_rect.h};
  DrawText(render, line_text, text_rect, GetDrawType());
}

}  // namespace client
}  // namespace fastotv
