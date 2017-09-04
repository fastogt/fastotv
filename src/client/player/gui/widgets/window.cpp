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

#include "client/player/gui/widgets/window.h"

#include <common/application/application.h>

#include "client/player/draw/draw.h"

namespace fastotv {
namespace client {
namespace player {
namespace gui {

Window::Window() : Window(draw::white_color) {}

Window::Window(const SDL_Color& back_ground_color)
    : rect_(draw::empty_rect),
      back_ground_color_(back_ground_color),
      border_color_(draw::black_color),
      visible_(false),
      transparent_(false),
      bordered_(false),
      focus_(false),
      enabled_(true),
      min_size_(draw::invalid_size),
      mouse_clicked_cb_(),
      mouse_released_cb_(),
      focus_changed_cb_(),
      enable_chaned_cb_() {
  Init();
}

Window::~Window() {}

void Window::SetRect(const SDL_Rect& rect) {
  rect_ = rect;
}

void Window::SetMouseClickedCallback(mouse_clicked_callback_t cb) {
  mouse_clicked_cb_ = cb;
}

void Window::SetMouseReeleasedCallback(mouse_released_callback_t cb) {
  mouse_released_cb_ = cb;
}

void Window::SetFocusChangedCallback(focus_changed_callback_t cb) {
  focus_changed_cb_ = cb;
}

void Window::SetEnableChangedCallback(enable_changed_callback_t cb) {
  enable_chaned_cb_ = cb;
}

SDL_Rect Window::GetRect() const {
  /*if(draw::IsEmptyRect(rect_)) {
    DNOTREACHED();
  }*/
  return rect_;
}

SDL_Color Window::GetBackGroundColor() const {
  return back_ground_color_;
}

void Window::SetBackGroundColor(const SDL_Color& color) {
  back_ground_color_ = color;
}

SDL_Color Window::GetBorderColor() const {
  return border_color_;
}

void Window::SetBorderColor(const SDL_Color& color) {
  border_color_ = color;
}

void Window::SetMinimalSize(const draw::Size& ms) {
  min_size_ = ms;
}

draw::Size Window::GetMinimalSize() const {
  return min_size_;
}

void Window::SetTransparent(bool t) {
  transparent_ = t;
}

bool Window::IsTransparent() const {
  return transparent_;
}

bool Window::IsBordered() const {
  return bordered_;
}

void Window::SetBordered(bool b) {
  bordered_ = b;
}

void Window::SetVisible(bool v) {
  visible_ = v;
  if (!visible_) {
    SetFocus(false);
  }
}

bool Window::IsVisible() const {
  return visible_;
}

void Window::SetEnabled(bool en) {
  enabled_ = en;
  if (!enabled_) {
    SetFocus(false);
  }
  OnEnabledChanged(en);
}

bool Window::IsEnabled() const {
  return enabled_;
}

void Window::SetFocus(bool focus) {
  focus_ = focus;
  OnFocusChanged(focus);
}

bool Window::IsFocused() const {
  return focus_;
}

void Window::Show() {
  SetVisible(true);
}

void Window::Hide() {
  SetVisible(false);
}

void Window::ToggleVisible() {
  SetVisible(!IsVisible());
}

void Window::Draw(SDL_Renderer* render) {
  if (!IsVisible() || !IsCanDraw()) {
    return;
  }

  if (!IsTransparent()) {
    draw::FillRectColor(render, rect_, back_ground_color_);
  }

  if (IsBordered()) {
    draw::DrawBorder(render, rect_, border_color_);
  }
}

void Window::Init() {
  fApp->Subscribe(this, gui::events::MouseStateChangeEvent::EventType);
  fApp->Subscribe(this, gui::events::MouseMoveEvent::EventType);
  fApp->Subscribe(this, gui::events::MousePressEvent::EventType);
  fApp->Subscribe(this, gui::events::MouseReleaseEvent::EventType);

  fApp->Subscribe(this, gui::events::WindowResizeEvent::EventType);
  fApp->Subscribe(this, gui::events::WindowExposeEvent::EventType);
  fApp->Subscribe(this, gui::events::WindowCloseEvent::EventType);
}

bool Window::IsCanDraw() const {
  if (min_size_ != draw::invalid_size) {
    if (min_size_.height > rect_.h || min_size_.width > rect_.w) {
      return false;
    }
  }

  return true;
}

bool Window::IsPointInControlArea(const SDL_Point& point) const {
  if (!IsVisible() || !IsCanDraw()) {
    return false;
  }

  return draw::IsPointInRect(point, rect_);
}

void Window::HandleEvent(event_t* event) {
  if (event->GetEventType() == gui::events::WindowResizeEvent::EventType) {
    gui::events::WindowResizeEvent* win_resize_event = static_cast<gui::events::WindowResizeEvent*>(event);
    HandleWindowResizeEvent(win_resize_event);
  } else if (event->GetEventType() == gui::events::WindowExposeEvent::EventType) {
    gui::events::WindowExposeEvent* win_expose = static_cast<gui::events::WindowExposeEvent*>(event);
    HandleWindowExposeEvent(win_expose);
  } else if (event->GetEventType() == gui::events::WindowCloseEvent::EventType) {
    gui::events::WindowCloseEvent* window_close = static_cast<gui::events::WindowCloseEvent*>(event);
    HandleWindowCloseEvent(window_close);
  } else if (event->GetEventType() == gui::events::MouseStateChangeEvent::EventType) {
    gui::events::MouseStateChangeEvent* mouse_state = static_cast<gui::events::MouseStateChangeEvent*>(event);
    HandleMouseStateChangeEvent(mouse_state);
  } else if (event->GetEventType() == gui::events::MouseMoveEvent::EventType) {
    gui::events::MouseMoveEvent* mouse_move = static_cast<gui::events::MouseMoveEvent*>(event);
    HandleMouseMoveEvent(mouse_move);
  } else if (event->GetEventType() == gui::events::MousePressEvent::EventType) {
    gui::events::MousePressEvent* mouse_press = static_cast<gui::events::MousePressEvent*>(event);
    HandleMousePressEvent(mouse_press);
  } else if (event->GetEventType() == gui::events::MouseReleaseEvent::EventType) {
    gui::events::MouseReleaseEvent* mouse_release = static_cast<gui::events::MouseReleaseEvent*>(event);
    HandleMouseReleaseEvent(mouse_release);
  }
}

void Window::HandleExceptionEvent(event_t* event, common::Error err) {
  UNUSED(event);
  UNUSED(err);
}

void Window::HandleWindowResizeEvent(gui::events::WindowResizeEvent* event) {
  UNUSED(event);
}

void Window::HandleWindowExposeEvent(gui::events::WindowExposeEvent* event) {
  UNUSED(event);
}

void Window::HandleWindowCloseEvent(gui::events::WindowCloseEvent* event) {
  UNUSED(event);
}

void Window::HandleMouseStateChangeEvent(gui::events::MouseStateChangeEvent* event) {
  if (!IsVisible() || !IsCanDraw()) {
    return;
  }

  if (!IsEnabled()) {
    return;
  }

  player::gui::events::MouseStateChangeInfo minf = event->GetInfo();
  if (!minf.IsCursorVisible()) {
    SetFocus(false);
    return;
  }

  SetFocus(IsPointInControlArea(minf.GetMousePoint()));
}

void Window::HandleMousePressEvent(gui::events::MousePressEvent* event) {
  if (!IsVisible() || !IsCanDraw()) {
    return;
  }

  if (!IsEnabled()) {
    return;
  }

  player::gui::events::MousePressInfo inf = event->GetInfo();
  SDL_Point point = inf.GetMousePoint();
  if (IsPointInControlArea(point)) {
    OnMouseClicked(inf.mevent.button, point);
  }
}

void Window::HandleMouseReleaseEvent(events::MouseReleaseEvent* event) {
  if (!IsVisible() || !IsCanDraw()) {
    return;
  }

  if (!IsEnabled()) {
    return;
  }

  player::gui::events::MouseReleaseInfo inf = event->GetInfo();
  SDL_Point point = inf.GetMousePoint();
  if (IsPointInControlArea(point)) {
    OnMouseReleased(inf.mevent.button, point);
  }
}

void Window::HandleMouseMoveEvent(gui::events::MouseMoveEvent* event) {
  if (!IsVisible() || !IsCanDraw()) {
    return;
  }

  if (!IsEnabled()) {
    return;
  }

  player::gui::events::MouseMoveInfo minf = event->GetInfo();
  SetFocus(IsPointInControlArea(minf.GetMousePoint()));
}

void Window::OnMouseClicked(Uint8 button, const SDL_Point& position) {
  if (mouse_clicked_cb_) {
    mouse_clicked_cb_(button, position);
  }
}

void Window::OnMouseReleased(Uint8 button, const SDL_Point& position) {
  if (mouse_released_cb_) {
    mouse_released_cb_(button, position);
  }
}

void Window::OnEnabledChanged(bool enable) {
  if (enable_chaned_cb_) {
    enable_chaned_cb_(enable);
  }
}

void Window::OnFocusChanged(bool focus) {
  if (focus_changed_cb_) {
    focus_changed_cb_(focus);
  }
}

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
