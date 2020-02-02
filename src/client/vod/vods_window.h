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

#pragma once

#include <string>
#include <vector>

#include <player/gui/widgets/list_box.h>

#include "client/vod/vod_entry.h"

namespace fastotv {
namespace client {

class VodsWindow : public fastoplayer::gui::IListBox {
 public:
  typedef fastoplayer::gui::IListBox base_class;
  typedef std::vector<VodEntry> playlist_t;
  enum { channel_number_width = 60, space_width = 10 };
  explicit VodsWindow(const SDL_Color& back_ground_color, Window* parent = nullptr);
  ~VodsWindow() override;

  void SetPlaylist(const playlist_t* pl);
  const playlist_t* GetPlaylist() const;

  size_t GetRowCount() const override;

 protected:
  void DrawRow(SDL_Renderer* render, size_t pos, bool active, bool hover, const SDL_Rect& row_rect) override;

 private:
  const playlist_t* play_list_;  // pointer
};

}  // namespace client
}  // namespace fastotv
