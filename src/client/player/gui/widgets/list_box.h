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

#include "client/player/gui/widgets/font_window.h"

namespace fastotv {
namespace client {
namespace player {
namespace gui {

class IListBox : public FontWindow {
 public:
  typedef FontWindow base_class;
  typedef std::function<void(Uint8 button, size_t row)> mouse_clicked_row_callback_t;
  typedef std::function<void(Uint8 button, size_t row)> mouse_released_row_callback_t;

  enum Selection { NO_SELECT, SINGLE_ROW_SELECT };

  IListBox();
  IListBox(const SDL_Color& back_ground_color);
  ~IListBox();

  void SetMouseClickedRowCallback(mouse_clicked_row_callback_t cb);
  void SetMouseReeleasedRowCallback(mouse_clicked_row_callback_t cb);

  void SetSelection(Selection sel);
  int GetSelection() const;

  void SetSelectionColor(const SDL_Color& sel);
  SDL_Color GetSelectionColor() const;

  void SetRowHeight(int row_height);
  int GetRowHeight() const;

  virtual size_t GetRowCount() const = 0;
  virtual void Draw(SDL_Renderer* render) override;

 protected:
  virtual void DrawRow(SDL_Renderer* render, size_t pos, bool is_active_row, const SDL_Rect& row_rect) = 0;
  virtual void HandleMouseMoveEvent(gui::events::MouseMoveEvent* event) override;

  virtual void OnFocusChanged(bool focus) override;
  virtual void OnMouseClicked(Uint8 button, const SDL_Point& position) override;
  virtual void OnMouseReleased(Uint8 button, const SDL_Point& position) override;

 private:
  size_t FindRowInPosition(const SDL_Point& position) const;

  int row_height_;
  size_t last_drawed_row_pos_;

  Selection selection_;
  SDL_Color selection_color_;
  size_t preselected_row_;

  mouse_clicked_row_callback_t mouse_clicked_row_cb_;
  mouse_released_row_callback_t mouse_released_row_cb_;
};

class ListBox : public IListBox {
 public:
  typedef IListBox base_class;
  typedef std::vector<std::string> lines_t;

  ListBox();
  ListBox(const SDL_Color& back_ground_color);
  ~ListBox();

  void SetLines(const lines_t& lines);
  lines_t GetLines() const;

  virtual size_t GetRowCount() const override;

 protected:
  virtual void DrawRow(SDL_Renderer* render, size_t pos, bool is_active_row, const SDL_Rect& row_rect) override;

 private:
  lines_t lines_;
};

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
