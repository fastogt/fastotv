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

#include <player/gui/widgets/window.h>

#include "client/playlist_window.h"

namespace fastoplayer {
namespace gui {
class Button;
class LineEdit;
}  // namespace gui
}  // namespace fastoplayer

namespace fastotv {
namespace client {

class ProgramsWindow : public fastoplayer::gui::Window {
 public:
  typedef fastoplayer::gui::Window base_class;
  static const SDL_Color text_background_color;

  ProgramsWindow(const SDL_Color& back_ground_color);
  ~ProgramsWindow();

  bool IsActived() const;

  void SetMouseClickedRowCallback(PlaylistWindow::mouse_clicked_row_callback_t cb);

  void SetPlaylist(const PlaylistWindow::playlist_t* pl);

  void SetTextColor(const SDL_Color& color);

  void SetSelection(PlaylistWindow::Selection sel);

  void SetFont(TTF_Font* font);

  void SetRowHeight(int row_height);

  void SetSelectionColor(const SDL_Color& sel);

  void SetDrawType(fastoplayer::gui::FontWindow::DrawType dt);

  void SetCurrentPositionSelectionColor(const SDL_Color& sel);

  void SetCurrentPositionInPlaylist(size_t pos);

  virtual void Draw(SDL_Renderer* render) override;

 private:
  SDL_Rect GetPlaylistRect() const;
  SDL_Rect GetTextInputRect() const;

  PlaylistWindow* plailist_window_;
  fastoplayer::gui::LineEdit* text_input_box_;

  TTF_Font* font_;
  SDL_Color text_color_;
  // filters
  PlaylistWindow::mouse_clicked_row_callback_t proxy_clicked_cb_;
  const PlaylistWindow::playlist_t* origin_;
  PlaylistWindow::playlist_t filtered_origin_;
};

}  // namespace client
}  // namespace fastotv
