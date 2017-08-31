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

#include "client/player/gui/sdl2_application.h"

#include "client/player/draw/draw.h"

namespace fastotv {
namespace client {
namespace player {
namespace gui {

Window::Window()
    : rect_(draw::empty_rect),
      back_ground_color_(draw::white_color),
      visible_(true),
      transparent_(false),
      focus_(false) {
  fApp->Subscribe(this, gui::events::MouseMoveEvent::EventType);
  fApp->Subscribe(this, gui::events::MousePressEvent::EventType);

  fApp->Subscribe(this, gui::events::WindowResizeEvent::EventType);
  fApp->Subscribe(this, gui::events::WindowExposeEvent::EventType);
  fApp->Subscribe(this, gui::events::WindowCloseEvent::EventType);
}

Window::~Window() {}

void Window::SetRect(const SDL_Rect& rect) {
  rect_ = rect;
}

SDL_Rect Window::GetRect() const {
  return rect_;
}

SDL_Color Window::GetBackGroundColor() const {
  return back_ground_color_;
}

void Window::SetBackGroundColor(const SDL_Color& color) {
  back_ground_color_ = color;
}

void Window::SetTransparent(bool t) {
  transparent_ = t;
}

bool Window::IsTransparent() const {
  return transparent_;
}

void Window::SetVisible(bool v) {
  visible_ = v;
  if (!visible_) {
    focus_ = false;
  }
}

bool Window::IsVisible() const {
  return visible_;
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

void Window::Draw(SDL_Renderer* render) {
  if (!visible_) {
    return;
  }

  if (transparent_) {
    return;
  }

  draw::FillRectColor(render, rect_, back_ground_color_);
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
  } else if (event->GetEventType() == gui::events::MouseMoveEvent::EventType) {
    gui::events::MouseMoveEvent* mouse_move = static_cast<gui::events::MouseMoveEvent*>(event);
    HandleMouseMoveEvent(mouse_move);
  } else if (event->GetEventType() == gui::events::MousePressEvent::EventType) {
    gui::events::MousePressEvent* mouse_press = static_cast<gui::events::MousePressEvent*>(event);
    HandleMousePressEvent(mouse_press);
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

void Window::HandleMousePressEvent(gui::events::MousePressEvent* event) {
  UNUSED(event);
}

void Window::HandleMouseMoveEvent(gui::events::MouseMoveEvent* event) {
  if (!visible_) {
    return;
  }

  player::gui::events::MouseMoveInfo minf = event->GetInfo();
  focus_ = draw::IsPointInRect(minf.GetMousePoint(), rect_);
}

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
