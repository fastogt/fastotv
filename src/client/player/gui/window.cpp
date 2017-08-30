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

Window::Window() : visible_(true), focus_(false) {
  fApp->Subscribe(this, core::events::MouseMoveEvent::EventType);
  fApp->Subscribe(this, core::events::MousePressEvent::EventType);

  fApp->Subscribe(this, core::events::WindowResizeEvent::EventType);
  fApp->Subscribe(this, core::events::WindowExposeEvent::EventType);
  fApp->Subscribe(this, core::events::WindowCloseEvent::EventType);
}

Window::~Window() {}

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

void Window::Draw(SDL_Renderer* render, const SDL_Rect& rect, const SDL_Color& back_ground_color) {
  if (!visible_) {
    return;
  }

  draw::FillRectColor(render, rect, back_ground_color);
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

void Window::HandleWindowResizeEvent(core::events::WindowResizeEvent* event) {}

void Window::HandleWindowExposeEvent(core::events::WindowExposeEvent* event) {}

void Window::HandleWindowCloseEvent(core::events::WindowCloseEvent* event) {}

void Window::HandleMousePressEvent(core::events::MousePressEvent* event) {}

void Window::HandleMouseMoveEvent(core::events::MouseMoveEvent* event) {}

}  // namespace core
}  // namespace player
}  // namespace client
}  // namespace fastotv
