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

#include <SDL2/SDL_render.h>

#include "client/player/draw/types.h"

#include "client/player/gui/events/window_events.h"
#include "client/player/gui/events/mouse_events.h"

namespace fastotv {
namespace client {
namespace player {
namespace gui {

class Window : public core::events::EventListener {
 public:
  Window();
  virtual ~Window();

  void SetRect(const SDL_Rect& rect);
  SDL_Rect GetRect() const;

  SDL_Color GetBackGroundColor() const;
  void SetBackGroundColor(const SDL_Color& color);

  void SetTransparent(bool t);
  bool IsTransparent() const;

  void SetVisible(bool v);
  bool IsVisible() const;

  void Show();
  void Hide();

  virtual void Draw(SDL_Renderer* render);

 protected:
  virtual void HandleEvent(event_t* event) override;
  virtual void HandleExceptionEvent(event_t* event, common::Error err) override;

  virtual void HandleWindowResizeEvent(core::events::WindowResizeEvent* event);
  virtual void HandleWindowExposeEvent(core::events::WindowExposeEvent* event);
  virtual void HandleWindowCloseEvent(core::events::WindowCloseEvent* event);

  virtual void HandleMousePressEvent(core::events::MousePressEvent* event);
  virtual void HandleMouseMoveEvent(core::events::MouseMoveEvent* event);

 private:
  SDL_Rect rect_;
  SDL_Color back_ground_color_;
  bool visible_;
  bool transparent_;
  bool focus_;
};

}  // namespace core
}  // namespace player
}  // namespace client
}  // namespace fastotv
