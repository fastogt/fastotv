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

#include "client/player/gui/widgets/list_box.h"

#include "client/playlist_entry.h"

namespace fastotv {
namespace client {

class PlaylistWindow : public player::gui::IListBox {
 public:
  typedef player::gui::IListBox base_class;
  typedef std::vector<PlaylistEntry> playlist_t;
  enum { keypad_width = 60, space_width = 10 };
  PlaylistWindow();
  ~PlaylistWindow();

  void SetPlaylist(const playlist_t* pl);
  const playlist_t* GetPlaylist() const;

  void SetHideButtonImage(SDL_Texture* img);

  void SetShowButtonImage(SDL_Texture* img);
  virtual size_t GetRowCount() const override;

  virtual void Draw(SDL_Renderer* render) override;

  void SetCurrentPositionInPlaylist(size_t pos);

  void SetCurrentPositionSelectionColor(const SDL_Color& sel);
  SDL_Color GetCurrentPositionSelectionColor() const;

 protected:
  virtual void HandleMousePressEvent(player::gui::events::MousePressEvent* event) override;
  virtual void DrawRow(SDL_Renderer* render, size_t pos, bool is_active_row, const SDL_Rect& row_rect) override;

 private:
  bool IsHideButtonProgramsListRect(const SDL_Point& point) const;
  bool IsShowButtonProgramsListRect(const SDL_Point& point) const;
  SDL_Rect GetHideButtonProgramsListRect() const;
  SDL_Rect GetShowButtonProgramsListRect() const;

  SDL_Texture* hide_button_img_;
  SDL_Texture* show_button_img_;
  const playlist_t* play_list_;  // pointer

  size_t current_position_in_playlist_;
  SDL_Color select_cur_color_;
};

}  // namespace client
}  // namespace fastotv
