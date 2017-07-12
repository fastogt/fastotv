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

#include "client/core/application/sdl2_application.h"

#include <stdlib.h>  // for EXIT_SUCCESS, EXIT_FAI...

#include <SDL2/SDL.h>           // for SDL_Init, SDL_Quit
#include <SDL2/SDL_keyboard.h>  // for SDL_Keysym
#include <SDL2/SDL_mouse.h>     // for SDL_ShowCursor
#include <SDL2/SDL_stdinc.h>    // for Uint32
#include <SDL2/SDL_timer.h>     // for SDL_AddTimer, SDL_Remo...
#include <SDL2/SDL_ttf.h>       // for TTF_Init, TTF_Quit
#include <SDL2/SDL_video.h>     // for ::SDL_WINDOWEVENT_CLOSE

#include <common/logger.h>                  // for COMPACT_LOG_ERROR, COM...
#include <common/macros.h>                  // for UNUSED, DNOTREACHED
#include <common/threads/thread_manager.h>  // for THREAD_MANAGER

#include "client/core/events/events.h"  // for QuitEvent, QuitInfo

#include "client/core/types.h"  // for GetCurrentMsec, msec_t
#include "client/types.h"       // for Size

#define FASTO_EVENT (SDL_USEREVENT)

namespace {
Keysym SDLKeySymToOur(SDL_Keysym sks) {
  Keysym ks;
  ks.mod = sks.mod;
  ks.scancode = static_cast<Scancode>(sks.scancode);
  ks.sym = sks.sym;
  return ks;
}

template <typename T>
bool InRange(T a, T amin, T amax) {
  return amin <= a && a <= amax;
}

}  // namespace

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
namespace application {

Sdl2Application::Sdl2Application(int argc, char** argv)
    : common::application::IApplicationImpl(argc, argv), dispatcher_() {}

Sdl2Application::~Sdl2Application() {
  THREAD_MANAGER()->FreeInstance();
}

int Sdl2Application::PreExec() {
  Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
  int res = SDL_Init(flags);
  if (res != 0) {
    ERROR_LOG() << "Could not initialize SDL error: " << SDL_GetError();
    return EXIT_FAILURE;
  }

  SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
  SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

  res = TTF_Init();
  if (res != 0) {
    ERROR_LOG() << "SDL_ttf could not error: " << TTF_GetError();
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int Sdl2Application::Exec() {
  while (true) {
    SDL_Event event;
    SDL_PumpEvents();
    Uint32 start_wait_ts = SDL_GetTicks();
    int res = SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
    if (res == -1) {        // error
    } else if (res == 0) {  // no events
      events::TimeInfo inf;
      events::TimerEvent* timer_event = new events::TimerEvent(this, inf);
      HandleEvent(timer_event);
      int work_time = SDL_GetTicks() - start_wait_ts;
      int sleep_timeout = event_timeout_wait_msec - work_time;
      if (InRange<int>(sleep_timeout, 0, event_timeout_wait_msec)) {
        SDL_Delay(sleep_timeout);
      }
    } else {  // some events
      bool is_stop_event = event.type == FASTO_EVENT && event.user.data1 == NULL;
      if (is_stop_event) {
        break;
      }

      ProcessEvent(&event);
    }
  }

  return EXIT_SUCCESS;
}

void Sdl2Application::ProcessEvent(SDL_Event* event) {
  switch (event->type) {
    case SDL_KEYDOWN: {
      HandleKeyDownEvent(&event->key);
      break;
    }
    case SDL_KEYUP: {
      HandleKeyUpEvent(&event->key);
      break;
    }
    case SDL_MOUSEBUTTONDOWN: {
      HandleMousePressEvent(&event->button);
      break;
    }
    case SDL_MOUSEBUTTONUP: {
      HandleMouseReleaseEvent(&event->button);
      break;
    }
    case SDL_MOUSEMOTION: {
      HandleMouseMoveEvent(&event->motion);
      break;
    }
    case SDL_WINDOWEVENT: {
      HandleWindowEvent(&event->window);
      break;
    }
    case SDL_QUIT: {
      HandleQuitEvent(&event->quit);
      break;
    }
    case FASTO_EVENT: {
      events::Event* fevent = static_cast<events::Event*>(event->user.data1);
      HandleEvent(fevent);
      break;
    }
    default:
      break;
  }
}

int Sdl2Application::PostExec() {
  TTF_Quit();
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == FASTO_EVENT) {
      events::Event* fevent = static_cast<events::Event*>(event.user.data1);
      delete fevent;
    }
  }
  SDL_Quit();
  return EXIT_SUCCESS;
}

void Sdl2Application::Subscribe(common::IListener* listener, common::events_size_t id) {
  dispatcher_.Subscribe(static_cast<events::EventListener*>(listener), id);
}

void Sdl2Application::UnSubscribe(common::IListener* listener, common::events_size_t id) {
  dispatcher_.UnSubscribe(static_cast<events::EventListener*>(listener), id);
}

void Sdl2Application::UnSubscribe(common::IListener* listener) {
  dispatcher_.UnSubscribe(static_cast<events::EventListener*>(listener));
}

void Sdl2Application::SendEvent(common::IEvent* event) {
  if (THREAD_MANAGER()->IsMainThread()) {
    events::Event* fevent = static_cast<events::Event*>(event);
    HandleEvent(fevent);
    return;
  }

  DNOTREACHED();
  PostEvent(event);
}

void Sdl2Application::PostEvent(common::IEvent* event) {
  SDL_Event sevent;
  sevent.type = FASTO_EVENT;
  sevent.user.data1 = event;
  int res = SDL_PushEvent(&sevent);
  if (res != 1) {
    DNOTREACHED();
    delete event;
  }
}

void Sdl2Application::Exit(int result) {
  UNUSED(result);
  PostEvent(NULL);  // FIX ME
}

void Sdl2Application::ShowCursor() {
  SDL_ShowCursor(1);
}

void Sdl2Application::HideCursor() {
  SDL_ShowCursor(0);
}

common::application::timer_id_t Sdl2Application::AddTimer(uint32_t interval,
                                                          common::application::timer_callback_t cb,
                                                          void* user_data) {
  return SDL_AddTimer(interval, cb, user_data);
}

bool Sdl2Application::RemoveTimer(common::application::timer_id_t id) {
  return SDL_RemoveTimer(id);
}

void Sdl2Application::HandleEvent(events::Event* event) {
  // bool is_filtered_event = event_type == PRE_EXEC_EVENT || event_type == POST_EXEC_EVENT;
  dispatcher_.ProcessEvent(event);
}

void Sdl2Application::HandleKeyDownEvent(SDL_KeyboardEvent* event) {
  Keysym ks = SDLKeySymToOur(event->keysym);  // && event->repeat == 0
  events::KeyPressInfo inf(event->state == SDL_PRESSED, ks);
  events::KeyPressEvent* key_press = new events::KeyPressEvent(this, inf);
  HandleEvent(key_press);
}

void Sdl2Application::HandleKeyUpEvent(SDL_KeyboardEvent* event) {
  Keysym ks = SDLKeySymToOur(event->keysym);
  events::KeyReleaseInfo inf(event->state == SDL_PRESSED, ks);
  events::KeyReleaseEvent* key_release = new events::KeyReleaseEvent(this, inf);
  HandleEvent(key_release);
}

void Sdl2Application::HandleWindowEvent(SDL_WindowEvent* event) {  // SDL_WindowEventID
  if (event->event == SDL_WINDOWEVENT_RESIZED) {
    Size new_size(event->data1, event->data2);
    events::WindowResizeInfo inf(new_size);
    events::WindowResizeEvent* wind_resize = new events::WindowResizeEvent(this, inf);
    HandleEvent(wind_resize);
  } else if (event->event == SDL_WINDOWEVENT_EXPOSED) {
    events::WindowExposeInfo inf;
    events::WindowExposeEvent* wind_exp = new events::WindowExposeEvent(this, inf);
    HandleEvent(wind_exp);
  } else if (event->event == SDL_WINDOWEVENT_CLOSE) {
    events::WindowCloseInfo inf;
    events::WindowCloseEvent* wind_close = new events::WindowCloseEvent(this, inf);
    HandleEvent(wind_close);
  }
}

void Sdl2Application::HandleMousePressEvent(SDL_MouseButtonEvent* event) {
  events::MousePressInfo inf(event->button, event->state);
  events::MousePressEvent* mouse_press_event = new events::MousePressEvent(this, inf);
  HandleEvent(mouse_press_event);
}

void Sdl2Application::HandleMouseReleaseEvent(SDL_MouseButtonEvent* event) {
  events::MouseReleaseInfo inf(event->button, event->state);
  events::MouseReleaseEvent* mouse_release_event = new events::MouseReleaseEvent(this, inf);
  HandleEvent(mouse_release_event);
}

void Sdl2Application::HandleMouseMoveEvent(SDL_MouseMotionEvent* event) {
  UNUSED(event);

  events::MouseMoveInfo inf;
  events::MouseMoveEvent* mouse_move_event = new events::MouseMoveEvent(this, inf);
  HandleEvent(mouse_move_event);
}

void Sdl2Application::HandleQuitEvent(SDL_QuitEvent* event) {
  UNUSED(event);

  events::QuitInfo inf;
  events::QuitEvent* quit_event = new events::QuitEvent(this, inf);
  HandleEvent(quit_event);
}

Uint32 Sdl2Application::timer_callback(Uint32 interval, void* user_data) {
  Sdl2Application* app = static_cast<Sdl2Application*>(user_data);
  events::TimeInfo inf;
  events::TimerEvent* timer_event = new events::TimerEvent(app, inf);
  app->PostEvent(timer_event);
  return interval;
}

}  // namespace application
}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
