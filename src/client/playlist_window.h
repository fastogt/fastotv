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

#include <player/gui/widgets/list_box.h>

#include "client/playlist_entry.h"

namespace fastotv {
namespace client {

class PlaylistWindow : public fastoplayer::gui::IListBox {
 public:
  typedef fastoplayer::gui::IListBox base_class;
  typedef std::vector<PlaylistEntry> playlist_t;
  enum { channel_number_width = 60, space_width = 10 };
  PlaylistWindow(const SDL_Color& back_ground_color);
  ~PlaylistWindow();

  void SetPlaylist(const playlist_t* pl);
  const playlist_t* GetPlaylist() const;

  virtual size_t GetRowCount() const override;

  void SetCurrentPositionInPlaylist(size_t pos);

  void SetCurrentPositionSelectionColor(const SDL_Color& sel);
  SDL_Color GetCurrentPositionSelectionColor() const;

 protected:
  virtual void DrawRow(SDL_Renderer* render, size_t pos, bool is_active_row, const SDL_Rect& row_rect) override;

 private:
  const playlist_t* play_list_;  // pointer

  size_t current_position_in_playlist_;
  SDL_Color select_cur_color_;
};

}  // namespace client
}  // namespace fastotv
