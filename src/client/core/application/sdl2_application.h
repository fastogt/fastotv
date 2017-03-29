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

#include <SDL2/SDL_events.h>  // for SDL_EventState, SDL_IGNORE, etc

#include <common/application/application.h>
#include <common/threads/event_dispatcher.h>

#include "client/core/events/events_base.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
namespace application {

class Sdl2Application : public common::application::IApplicationImpl {
 public:
  enum { event_timeout_wait_msec = 1000 / 50 };  // 50 fps
  Sdl2Application(int argc, char** argv);
  ~Sdl2Application();

  virtual int PreExec() override;   // EXIT_FAILURE, EXIT_SUCCESS
  virtual int Exec() override;      // EXIT_FAILURE, EXIT_SUCCESS
  virtual int PostExec() override;  // EXIT_FAILURE, EXIT_SUCCESS

  virtual void Subscribe(common::IListener* listener, common::events_size_t id) override;
  virtual void UnSubscribe(common::IListener* listener, common::events_size_t id) override;
  virtual void UnSubscribe(common::IListener* listener) override;

  virtual void SendEvent(common::IEvent* event) override;
  virtual void PostEvent(common::IEvent* event) override;

  virtual void Exit(int result) override;

  virtual void ShowCursor() override;
  virtual void HideCursor() override;

 protected:
  virtual void HandleEvent(core::events::Event* event);

  virtual void HandleKeyPressEvent(SDL_KeyboardEvent* event);
  virtual void HandleWindowEvent(SDL_WindowEvent* event);

  virtual void HandleMouseMoveEvent(SDL_MouseMotionEvent* event);

  virtual void HandleMousePressEvent(SDL_MouseButtonEvent* event);
  virtual void HandleMouseReleaseEvent(SDL_MouseButtonEvent* event);

  virtual void HandleQuitEvent(SDL_QuitEvent* event);

 private:
  common::threads::EventDispatcher<EventsType> dispatcher_;
  bool stop_;
};
}
}
}
}
}
