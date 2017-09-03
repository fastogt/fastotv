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

#include "client/player/gui/events/mouse_events.h"
#include "client/player/gui/events/window_events.h"

namespace fastotv {
namespace client {
namespace player {
namespace gui {

class Window : public gui::events::EventListener {
 public:
  typedef std::function<void(Uint8 button, const SDL_Point& position)> mouse_clicked_callback_t;
  typedef std::function<void(Uint8 button, const SDL_Point& position)> mouse_released_callback_t;
  typedef std::function<void(bool focus)> focus_changed_callback_t;

  Window();
  Window(const SDL_Color& back_ground_color);
  virtual ~Window();

  void SetMouseClickedCallback(mouse_clicked_callback_t cb);
  void SetMouseReeleasedCallback(mouse_released_callback_t cb);
  void SetFocusChangedCallback(focus_changed_callback_t cb);

  void SetRect(const SDL_Rect& rect);
  SDL_Rect GetRect() const;

  SDL_Color GetBackGroundColor() const;
  void SetBackGroundColor(const SDL_Color& color);

  SDL_Color GetBorderColor() const;
  void SetBorderColor(const SDL_Color& color);

  draw::Size GetMinimalSize() const;
  void SetMinimalSize(const draw::Size& ms);

  bool IsTransparent() const;
  void SetTransparent(bool t);

  bool IsBordered() const;
  void SetBordered(bool b);

  void SetVisible(bool v);
  bool IsVisible() const;

  bool IsFocused() const;
  void SetFocus(bool focus);

  void Show();
  void Hide();
  void ToggleVisible();

  virtual void Draw(SDL_Renderer* render);

 protected:
  virtual void HandleEvent(event_t* event) override;
  virtual void HandleExceptionEvent(event_t* event, common::Error err) override;

  virtual void HandleWindowResizeEvent(gui::events::WindowResizeEvent* event);
  virtual void HandleWindowExposeEvent(gui::events::WindowExposeEvent* event);
  virtual void HandleWindowCloseEvent(gui::events::WindowCloseEvent* event);

  virtual void HandleMouseStateChangeEvent(gui::events::MouseStateChangeEvent* event);
  virtual void HandleMousePressEvent(gui::events::MousePressEvent* event);
  virtual void HandleMouseMoveEvent(gui::events::MouseMoveEvent* event);
  virtual void HandleMouseReleaseEvent(events::MouseReleaseEvent* event);

  virtual void OnFocusChanged(bool focus);
  virtual void OnMouseClicked(Uint8 button, const SDL_Point& position);
  virtual void OnMouseReleased(Uint8 button, const SDL_Point& position);

  bool IsCanDraw() const;
  bool IsPointInControlArea(const SDL_Point& point) const;

 private:
  void Init();

  void DrawBackground(SDL_Renderer* render);
  void DrawBorder(SDL_Renderer* render);

  SDL_Rect rect_;
  SDL_Color back_ground_color_;
  SDL_Color border_color_;

  bool visible_;
  bool transparent_;
  bool bordered_;
  bool focus_;
  draw::Size min_size_;

  mouse_clicked_callback_t mouse_clicked_cb_;
  mouse_released_callback_t mouse_released_cb_;
  focus_changed_callback_t focus_changed_cb_;
};

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
