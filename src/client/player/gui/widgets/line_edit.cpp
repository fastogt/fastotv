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

#include "client/player/gui/widgets/line_edit.h"

#include <common/application/application.h>
#include <common/time.h>

#include "client/player/draw/draw.h"

namespace fastotv {
namespace client {
namespace player {
namespace gui {

LineEdit::LineEdit() : base_class(), active_(false), start_blink_ts_(0), show_cursor_(false) {
  Init();
}

LineEdit::LineEdit(const SDL_Color& back_ground_color)
    : base_class(back_ground_color), active_(false), start_blink_ts_(0), show_cursor_(false) {
  Init();
}

LineEdit::~LineEdit() {}

bool LineEdit::IsActived() const {
  return active_;
}

void LineEdit::SetActived(bool active) {
  active_ = active;
  OnActiveChanged(active);
}

void LineEdit::Draw(SDL_Renderer* render) {
  if (!IsVisible() || !IsCanDraw()) {
    return;
  }

  SDL_Rect text_rect = GetRect();
  base_class::DrawLabel(render, &text_rect);

  if (active_) {
    common::time64_t cur_ts = common::time::current_mstime();
    if (start_blink_ts_ == 0) {
      start_blink_ts_ = cur_ts;
    }
    common::time64_t diff = cur_ts - start_blink_ts_;
    if (blinking_cursor_time_msec > diff) {
    } else {
      start_blink_ts_ = cur_ts;
      show_cursor_ = !show_cursor_;
    }

    if (show_cursor_) {
      int width_pos = text_.empty() ? cursor_width : text_rect.w;
      int cursor_height = text_rect.h - cursor_width * 2;
      SDL_Rect cursor_rect = {width_pos, text_rect.y + cursor_width, cursor_width, cursor_height};
      draw::FillRectColor(render, cursor_rect, draw::black_color);
    }
  }
}

void LineEdit::HandleEvent(event_t* event) {
  if (event->GetEventType() == gui::events::KeyPressEvent::EventType) {
    gui::events::KeyPressEvent* key_press_event = static_cast<gui::events::KeyPressEvent*>(event);
    HandleKeyPressEvent(key_press_event);
  } else if (event->GetEventType() == gui::events::KeyPressEvent::EventType) {
    gui::events::KeyReleaseEvent* key_rel_event = static_cast<gui::events::KeyReleaseEvent*>(event);
    HandleKeyReleaseEvent(key_rel_event);
  } else if (event->GetEventType() == gui::events::TextInputEvent::EventType) {
    gui::events::TextInputEvent* ti_event = static_cast<gui::events::TextInputEvent*>(event);
    HandleTextInputEvent(ti_event);
  } else if (event->GetEventType() == gui::events::TextEditEvent::EventType) {
    gui::events::TextEditEvent* ti_event = static_cast<gui::events::TextEditEvent*>(event);
    HandleTextEditEvent(ti_event);
  }

  base_class::HandleEvent(event);
}

void LineEdit::HandleExceptionEvent(event_t* event, common::Error err) {
  UNUSED(event);
  UNUSED(err);
}

void LineEdit::HandleMousePressEvent(player::gui::events::MousePressEvent* event) {
  if (!IsVisible() || !IsCanDraw()) {
    base_class::HandleMousePressEvent(event);
    return;
  }

  player::gui::events::MousePressInfo inf = event->GetInfo();
  SDL_Point point = inf.GetMousePoint();
  if (IsPointInControlArea(point)) {
    SetActived(true);
  } else {
    SetActived(false);
  }
  base_class::HandleMousePressEvent(event);
}

void LineEdit::HandleKeyPressEvent(gui::events::KeyPressEvent* event) {
  if (!IsVisible() || !IsCanDraw()) {
    return;
  }

  if (!active_) {
    return;
  }

  gui::events::KeyPressInfo kinf = event->GetInfo();
  if (kinf.ks.scancode == SDL_SCANCODE_RETURN) {  // escape press
    SetActived(false);
  } else if (kinf.ks.scancode == SDL_SCANCODE_ESCAPE) {  // escape press
    SetActived(false);
  } else if (kinf.ks.scancode == SDL_SCANCODE_BACKSPACE) {
    text_.pop_back();
  }
}

void LineEdit::HandleKeyReleaseEvent(gui::events::KeyReleaseEvent* event) {
  UNUSED(event);
  if (!IsVisible() || !IsCanDraw()) {
    return;
  }

  if (!active_) {
    return;
  }
}

void LineEdit::HandleTextInputEvent(gui::events::TextInputEvent* event) {
  UNUSED(event);
  if (!IsVisible() || !IsCanDraw()) {
    return;
  }

  if (!active_) {
    return;
  }

  gui::events::TextInputInfo tinf = event->GetInfo();
  int w = 0;
  int h = 0;
  SDL_Rect r = GetRect();
  std::string can_be_text = text_ + tinf.text;
  if (player::draw::GetTextSize(GetFont(), can_be_text, &w, &h) && w < r.w) {
    text_ = can_be_text;
  }
}

void LineEdit::HandleTextEditEvent(gui::events::TextEditEvent* event) {
  UNUSED(event);
  if (!IsVisible() || !IsCanDraw()) {
    return;
  }

  if (!active_) {
    return;
  }

  gui::events::TextEditInfo tinf = event->GetInfo();
  text_.append(tinf.text);
}

void LineEdit::Init() {
  fApp->Subscribe(this, gui::events::KeyPressEvent::EventType);
  fApp->Subscribe(this, gui::events::KeyReleaseEvent::EventType);
  fApp->Subscribe(this, gui::events::TextInputEvent::EventType);
  fApp->Subscribe(this, gui::events::TextEditEvent::EventType);
}

void LineEdit::OnActiveChanged(bool active) {
  if (active) {
    SDL_StartTextInput();
  } else {
    SDL_StopTextInput();
  }
}

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
