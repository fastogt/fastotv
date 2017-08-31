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

#include "client/player/gui/widgets/list_box.h"

#include "client/player/draw/draw.h"

namespace fastotv {
namespace client {
namespace player {
namespace gui {

IListBox::IListBox()
    : base_class(),
      row_height_(0),
      last_drawed_row_pos_(0),
      selection_(NO_SELECT),
      selection_color_(),
      active_row_(draw::invalid_row_position) {}

IListBox::~IListBox() {}

void IListBox::SetSelection(Selection sel) {
  selection_ = sel;
}

int IListBox::GetSelection() const {
  return selection_;
}

void IListBox::SetSelectionColor(const SDL_Color& sel) {
  selection_color_ = sel;
}

SDL_Color IListBox::GetSelectionColor() const {
  return selection_color_;
}

void IListBox::SetRowHeight(int row_height) {
  row_height_ = row_height;
}

int IListBox::GetRowHeight() const {
  return row_height_;
}

void IListBox::Draw(SDL_Renderer* render) {
  if (!IsVisible()) {
    return;
  }

  base_class::Draw(render);

  if (row_height_ == 0) {
    DNOTREACHED();
    return;
  }

  SDL_Rect draw_area = GetRect();
  int max_line_count = draw_area.h / row_height_;
  if (max_line_count == 0) {
    return;
  }

  bool have_active_row = active_row_ != draw::invalid_row_position;
  int drawed = 0;
  for (size_t i = last_drawed_row_pos_; i < GetRowCount() && drawed < max_line_count; ++i) {
    SDL_Rect cell_rect = {draw_area.x, draw_area.y + row_height_ * drawed, draw_area.w, row_height_};
    bool is_active_row = have_active_row && active_row_ == i;
    DrawRow(render, i, is_active_row, cell_rect);
    if (is_active_row && selection_ == SINGLE_ROW_SELECT) {
      draw::FillRectColor(render, cell_rect, selection_color_);
    }
    drawed++;
  }
}

size_t IListBox::GetActiveRow() const {
  return active_row_;
}

void IListBox::HandleMouseMoveEvent(gui::events::MouseMoveEvent* event) {
  if (!IsFocused()) {
    active_row_ = draw::invalid_row_position;
    base_class::HandleMouseMoveEvent(event);
    return;
  }

  gui::events::MouseMoveInfo minf = event->GetInfo();
  SDL_Point point = minf.GetMousePoint();
  SDL_Rect draw_area = GetRect();
  if (!draw::IsPointInRect(point, draw_area)) {  // not in rect
    active_row_ = draw::invalid_row_position;
    base_class::HandleMouseMoveEvent(event);
    return;
  }

  if (row_height_ == 0) {
    DNOTREACHED();
    return;
  }

  int max_line_count = draw_area.h / row_height_;
  if (max_line_count == 0) {  // nothing draw
    active_row_ = draw::invalid_row_position;
    base_class::HandleMouseMoveEvent(event);
    return;
  }

  int drawed = 0;
  for (size_t i = last_drawed_row_pos_; i < GetRowCount() && drawed < max_line_count; ++i) {
    SDL_Rect cell_rect = {draw_area.x, draw_area.y + row_height_ * drawed, draw_area.w, row_height_};
    if (draw::IsPointInRect(point, cell_rect)) {
      active_row_ = i;
      break;
    }
    drawed++;
  }
  base_class::HandleMouseMoveEvent(event);
}

void IListBox::OnFocusChanged(bool focus) {
  if (!focus) {
    active_row_ = draw::invalid_row_position;
  }

  base_class::OnFocusChanged(focus);
}

ListBox::ListBox() : base_class(), lines_() {}

ListBox::~ListBox() {}

void ListBox::SetLines(const lines_t& lines) {
  lines_ = lines;
}

ListBox::lines_t ListBox::GetLines() const {
  return lines_;
}

size_t ListBox::GetRowCount() const {
  return lines_.size();
}

void ListBox::DrawRow(SDL_Renderer* render, size_t pos, bool is_active_row, const SDL_Rect& row_rect) {
  UNUSED(is_active_row);
  DrawText(render, lines_[pos], row_rect, GetDrawType());
}

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
