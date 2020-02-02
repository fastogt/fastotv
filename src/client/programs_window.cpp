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

#include "client/programs_window.h"

#include <string>

#include <player/gui/widgets/line_edit.h>

#include "client/live_stream/playlist_window.h"

#define SEARCH_PLACEHOLDER "Search..."

namespace fastotv {
namespace client {

const SDL_Color ProgramsWindow::text_background_color = fastoplayer::draw::white_color;

ProgramsWindow::ProgramsWindow(const SDL_Color& back_ground_color)
    : base_class(),
      plailist_window_(nullptr),
      text_input_box_(nullptr),
      font_(nullptr),
      text_color_(),
      proxy_clicked_cb_(),
      origin_(nullptr),
      filtered_origin_() {
  SetTransparent(true);

  // playlist window
  plailist_window_ = new PlaylistWindow(back_ground_color, this);
  plailist_window_->SetVisible(true);
  plailist_window_->SetPlaylist(&filtered_origin_);

  text_input_box_ = new fastoplayer::gui::LineEdit(text_background_color, this);
  text_input_box_->SetTextColor(fastoplayer::draw::black_color);
  text_input_box_->SetDrawType(fastoplayer::gui::Label::WRAPPED_TEXT);
  text_input_box_->SetVisible(true);
  text_input_box_->SetBorderColor(fastoplayer::draw::black_color);
  text_input_box_->SetBordered(true);
  text_input_box_->SetEnabled(true);
  text_input_box_->SetPlaceHolder(SEARCH_PLACEHOLDER);
  auto search_text_changed_cb = [this](const std::string& text) {
    filtered_origin_.clear();
    if (!origin_) {
      return;
    }

    for (size_t i = 0; i < origin_->size(); ++i) {
      PlaylistEntry ent = origin_->operator[](i);
      commands_info::ChannelInfo cinf = ent.GetChannelInfo();
      const commands_info::EpgInfo epg = cinf.GetEpg();
      std::string name = epg.GetDisplayName();
      if (text.empty() || name.find(text) != std::string::npos) {
        filtered_origin_.push_back(ent);
      }
    }
  };
  text_input_box_->SetTextChangedCallback(search_text_changed_cb);

  auto mouse_clicked_cb = [this](Uint8 button, size_t row) {
    if (proxy_clicked_cb_) {
      if (!origin_) {
        return;
      }

      PlaylistEntry ent = filtered_origin_[row];
      commands_info::ChannelInfo ent_inf = ent.GetChannelInfo();
      for (size_t i = 0; i < origin_->size(); ++i) {
        PlaylistEntry cent = origin_->operator[](i);
        if (cent.GetChannelInfo() == ent_inf) {
          proxy_clicked_cb_(button, i);
          return;
        }
      }
    }
  };
  plailist_window_->SetMouseClickedRowCallback(mouse_clicked_cb);
}

ProgramsWindow::~ProgramsWindow() {
  destroy(&text_input_box_);
  destroy(&plailist_window_);
}

bool ProgramsWindow::IsActived() const {
  return text_input_box_->IsActived();
}

void ProgramsWindow::SetPlaylist(const PlaylistWindow::playlist_t* pl) {
  origin_ = pl;
  text_input_box_->ClearText();
}

void ProgramsWindow::SetTextColor(const SDL_Color& color) {
  plailist_window_->SetTextColor(color);
  text_color_ = color;
}

void ProgramsWindow::SetSelection(PlaylistWindow::Selection sel) {
  plailist_window_->SetSelection(sel);
}

void ProgramsWindow::SetFont(TTF_Font* font) {
  plailist_window_->SetFont(font);
  text_input_box_->SetFont(font);
  font_ = font;
}

void ProgramsWindow::SetRowHeight(int row_height) {
  plailist_window_->SetRowHeight(row_height);
}

void ProgramsWindow::SetSelectionColor(const SDL_Color& sel) {
  plailist_window_->SetSelectionColor(sel);
}

void ProgramsWindow::SetDrawType(fastoplayer::gui::FontWindow::DrawType dt) {
  plailist_window_->SetDrawType(dt);
}

void ProgramsWindow::SetCurrentPositionSelectionColor(const SDL_Color& sel) {
  plailist_window_->SetActiveRowColor(sel);
}

void ProgramsWindow::SetCurrentPositionInPlaylist(size_t pos) {
  plailist_window_->SetActiveRow(pos);
}

void ProgramsWindow::SetMouseClickedRowCallback(PlaylistWindow::mouse_clicked_row_callback_t cb) {
  proxy_clicked_cb_ = cb;
}

void ProgramsWindow::Draw(SDL_Renderer* render) {
  if (!IsCanDraw()) {
    base_class::Draw(render);
    return;
  }

  base_class::Draw(render);

  plailist_window_->SetRect(GetPlaylistRect());
  plailist_window_->Draw(render);

  text_input_box_->SetRect(GetTextInputRect());
  text_input_box_->Draw(render);
}

SDL_Rect ProgramsWindow::GetPlaylistRect() const {
  if (!font_) {
    return fastoplayer::draw::empty_rect;
  }

  int font_height_2line = fastoplayer::draw::CalcHeightFontPlaceByRowCount(font_, 1);
  SDL_Rect chat_rect = GetRect();
  int button_height = font_height_2line;
  SDL_Rect chat_rect_without_filter_rect = {chat_rect.x, chat_rect.y, chat_rect.w, chat_rect.h - button_height};
  return chat_rect_without_filter_rect;
}

SDL_Rect ProgramsWindow::GetTextInputRect() const {
  if (!font_) {
    return fastoplayer::draw::empty_rect;
  }

  int font_height_2line = fastoplayer::draw::CalcHeightFontPlaceByRowCount(font_, 1);
  SDL_Rect chat_rect = GetRect();
  SDL_Rect text_input_rect = {chat_rect.x, chat_rect.y + chat_rect.h - font_height_2line, chat_rect.w,
                              font_height_2line};
  return text_input_rect;
}

}  // namespace client
}  // namespace fastotv
