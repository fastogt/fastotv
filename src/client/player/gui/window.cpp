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

#include "client/player/gui/window.h"

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
  fApp->Subscribe(this, core::events::MouseMoveEvent::EventType);
  fApp->Subscribe(this, core::events::MousePressEvent::EventType);

  fApp->Subscribe(this, core::events::WindowResizeEvent::EventType);
  fApp->Subscribe(this, core::events::WindowExposeEvent::EventType);
  fApp->Subscribe(this, core::events::WindowCloseEvent::EventType);
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
  if (event->GetEventType() == core::events::WindowResizeEvent::EventType) {
    core::events::WindowResizeEvent* win_resize_event = static_cast<core::events::WindowResizeEvent*>(event);
    HandleWindowResizeEvent(win_resize_event);
  } else if (event->GetEventType() == core::events::WindowExposeEvent::EventType) {
    core::events::WindowExposeEvent* win_expose = static_cast<core::events::WindowExposeEvent*>(event);
    HandleWindowExposeEvent(win_expose);
  } else if (event->GetEventType() == core::events::WindowCloseEvent::EventType) {
    core::events::WindowCloseEvent* window_close = static_cast<core::events::WindowCloseEvent*>(event);
    HandleWindowCloseEvent(window_close);
  } else if (event->GetEventType() == core::events::MouseMoveEvent::EventType) {
    core::events::MouseMoveEvent* mouse_move = static_cast<core::events::MouseMoveEvent*>(event);
    HandleMouseMoveEvent(mouse_move);
  } else if (event->GetEventType() == core::events::MousePressEvent::EventType) {
    core::events::MousePressEvent* mouse_press = static_cast<core::events::MousePressEvent*>(event);
    HandleMousePressEvent(mouse_press);
  }
}

void Window::HandleExceptionEvent(event_t* event, common::Error err) {
  UNUSED(event);
  UNUSED(err);
}

void Window::HandleWindowResizeEvent(core::events::WindowResizeEvent* event) {
  UNUSED(event);
}

void Window::HandleWindowExposeEvent(core::events::WindowExposeEvent* event) {
  UNUSED(event);
}

void Window::HandleWindowCloseEvent(core::events::WindowCloseEvent* event) {
  UNUSED(event);
}

void Window::HandleMousePressEvent(core::events::MousePressEvent* event) {
  UNUSED(event);
}

void Window::HandleMouseMoveEvent(core::events::MouseMoveEvent* event) {
  if (!visible_) {
    return;
  }

  player::core::events::MouseMoveInfo minf = event->GetInfo();
  focus_ = draw::PointInRect(minf.GetMousePoint(), rect_);
}

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
