/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

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

#include "client/chat_window.h"

#include <common/application/application.h>
#include <common/convert2string.h>

#include <player/draw/draw.h>
#include <player/gui/widgets/button.h>
#include <player/gui/widgets/line_edit.h>

#include "client/chat_list_window.h"

#define MESSAGE_PLACEHOLDER "Type a message"

namespace fastotv {
namespace client {

const SDL_Color ChatWindow::text_background_color = fastoplayer::draw::white_color;

ChatWindow::ChatWindow(const SDL_Color& back_ground_color)
    : base_class(),
      watchers_(0),
      chat_window_(nullptr),
      send_message_button_(nullptr),
      text_input_box_(nullptr),
      font_(nullptr),
      text_color_(),
      post_message_enabled_(false) {
  SetTransparent(true);

  chat_window_ = new ChatListWindow(back_ground_color);
  chat_window_->SetVisible(true);

  send_message_button_ = new fastoplayer::gui::Button(fastoplayer::draw::blue_color);
  send_message_button_->SetText("Post");
  send_message_button_->SetDrawType(fastoplayer::gui::Label::CENTER_TEXT);
  send_message_button_->SetVisible(true);
  send_message_button_->SetBorderColor(fastoplayer::draw::black_color);
  send_message_button_->SetBordered(true);

  text_input_box_ = new fastoplayer::gui::LineEdit(text_background_color);
  text_input_box_->SetTextColor(fastoplayer::draw::black_color);
  text_input_box_->SetDrawType(fastoplayer::gui::Label::WRAPPED_TEXT);
  text_input_box_->SetVisible(true);
  text_input_box_->SetBorderColor(fastoplayer::draw::black_color);
  text_input_box_->SetBordered(true);
  text_input_box_->SetPlaceHolder(MESSAGE_PLACEHOLDER);
}

ChatWindow::~ChatWindow() {
  destroy(&text_input_box_);
  destroy(&send_message_button_);
  destroy(&chat_window_);
}

std::string ChatWindow::GetInputText() const {
  return text_input_box_->GetText();
}

void ChatWindow::ClearInputText() const {
  text_input_box_->ClearText();
}

void ChatWindow::SetPostMessageEnabled(bool en) {
  post_message_enabled_ = en;
}

bool ChatWindow::IsActived() const {
  return text_input_box_->IsActived();
}

void ChatWindow::SetPostClickedCallback(mouse_clicked_callback_t cb) {
  send_message_button_->SetMouseClickedCallback(cb);
}

void ChatWindow::SetTextColor(const SDL_Color& color) {
  chat_window_->SetTextColor(color);
  send_message_button_->SetTextColor(color);
  text_color_ = color;
}

void ChatWindow::SetFont(TTF_Font* font) {
  chat_window_->SetFont(font);
  send_message_button_->SetFont(font);
  text_input_box_->SetFont(font);
  font_ = font;
}

void ChatWindow::SetWatchers(size_t watchers) {
  watchers_ = watchers;
}

void ChatWindow::SetMessages(const messages_t& msgs) {
  chat_window_->SetMessages(msgs);
}

void ChatWindow::SetRowHeight(int row_height) {
  chat_window_->SetRowHeight(row_height);
}

void ChatWindow::Draw(SDL_Renderer* render) {
  if (!IsCanDraw()) {
    base_class::Draw(render);
    return;
  }

  if (fApp->IsCursorVisible()) {
    SDL_Rect watchers_rect = GetWatcherRect();
    std::string watchers_str = common::ConvertToString(watchers_);
    fastoplayer::draw::FillRectColor(render, watchers_rect, fastoplayer::draw::red_color);
    fastoplayer::draw::DrawCenterTextInRect(render, watchers_str, font_, text_color_, watchers_rect);
  }

  base_class::Draw(render);

  chat_window_->SetRect(GetChatRect());
  chat_window_->Draw(render);

  text_input_box_->SetEnabled(post_message_enabled_);
  text_input_box_->SetRect(GetTextInputRect());
  text_input_box_->Draw(render);

  send_message_button_->SetEnabled(post_message_enabled_);
  send_message_button_->SetRect(GetSendButtonRect());
  send_message_button_->Draw(render);
}

SDL_Rect ChatWindow::GetWatcherRect() const {
  if (!font_) {
    return fastoplayer::draw::empty_rect;
  }

  int font_height_2line = fastoplayer::draw::CalcHeightFontPlaceByRowCount(font_, 2);
  SDL_Rect chat_rect = GetRect();
  SDL_Rect hide_button_rect = {chat_rect.w - font_height_2line, chat_rect.y - font_height_2line, font_height_2line,
                               font_height_2line};
  return hide_button_rect;
}

SDL_Rect ChatWindow::GetSendButtonRect() const {
  if (!font_) {
    return fastoplayer::draw::empty_rect;
  }

  int font_height_2line = fastoplayer::draw::CalcHeightFontPlaceByRowCount(font_, 1);
  SDL_Rect chat_rect = GetRect();
  SDL_Rect button_rect = {chat_rect.w - post_button_width, chat_rect.y + chat_rect.h - font_height_2line,
                          post_button_width, font_height_2line};
  return button_rect;
}

SDL_Rect ChatWindow::GetTextInputRect() const {
  if (!font_) {
    return fastoplayer::draw::empty_rect;
  }

  int font_height_2line = fastoplayer::draw::CalcHeightFontPlaceByRowCount(font_, 1);
  SDL_Rect chat_rect = GetRect();
  SDL_Rect text_input_rect = {chat_rect.x, chat_rect.y + chat_rect.h - font_height_2line,
                              chat_rect.w - post_button_width, font_height_2line};
  return text_input_rect;
}

SDL_Rect ChatWindow::GetChatRect() const {
  if (!font_) {
    return fastoplayer::draw::empty_rect;
  }

  int font_height_2line = fastoplayer::draw::CalcHeightFontPlaceByRowCount(font_, 1);
  SDL_Rect chat_rect = GetRect();
  int button_height = font_height_2line;
  SDL_Rect hide_button_rect = {chat_rect.x, chat_rect.y, chat_rect.w, chat_rect.h - button_height};
  return hide_button_rect;
}

}  // namespace client
}  // namespace fastotv
