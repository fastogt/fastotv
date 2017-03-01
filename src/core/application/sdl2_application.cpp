#include "core/application/sdl2_application.h"

#include <SDL2/SDL.h>         // for SDL_INIT_AUDIO, etc
#include <SDL2/SDL_error.h>   // for SDL_GetError
#include <SDL2/SDL_mutex.h>   // for SDL_CreateMutex, etc
#include <SDL2/SDL_stdinc.h>  // for SDL_getenv, SDL_setenv, etc

#include <common/threads/event_bus.h>

#include "core/events/events.h"

#define FF_ALLOC_EVENT (SDL_USEREVENT)
#define FF_QUIT_EVENT (SDL_USEREVENT + 2)
#define FF_NEXT_STREAM (SDL_USEREVENT + 3)
#define FF_PREV_STREAM (SDL_USEREVENT + 4)

#define FASTO_EVENT (SDL_USEREVENT)

namespace core {
namespace application {

Sdl2Application::Sdl2Application(int argc, char** argv)
    : common::application::IApplicationImpl(argc, argv), stop_(false), dispatcher_() {}

int Sdl2Application::PreExec() {
  Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
  if (SDL_Init(flags)) {
    ERROR_LOG() << "Could not initialize SDL - " << SDL_GetError();
    ERROR_LOG() << "(Did you set the DISPLAY variable?)";
    return EXIT_FAILURE;
  }

  SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
  SDL_EventState(SDL_USEREVENT, SDL_IGNORE);
  return EXIT_SUCCESS;
}

int Sdl2Application::Exec() {
  while (!stop_) {
    SDL_Event event;
    SDL_PumpEvents();
    int res = SDL_WaitEventTimeout(&event, event_timeout_wait_msec);
    if (res == 0) {  // timeout
      events::TimeInfo inf;
      events::TimerEvent* timer_event = new events::TimerEvent(this, inf);
      HandleEvent(timer_event);
      continue;
    }

    switch (event.type) {
      case SDL_KEYDOWN: {
        HandleKeyPressEvent(&event.key);
        break;
      }
      case SDL_MOUSEBUTTONDOWN: {
        HandleMousePressEvent(&event.button);
        break;
      }
      case SDL_MOUSEMOTION: {
        HandleMouseMoveEvent(&event.motion);
        break;
      }
      case SDL_WINDOWEVENT: {
        HandleWindowEvent(&event.window);
        break;
      }
      case FASTO_EVENT: {
        events::Event* fevent = static_cast<events::Event*>(event.user.data1);
        HandleEvent(fevent);
        break;
      }
      default:
        break;
    }
  }

  return EXIT_SUCCESS;
}

int Sdl2Application::PostExec() {
  SDL_Quit();
  return EXIT_SUCCESS;
}

void Sdl2Application::Subscribe(common::IListener* listener, common::events_size_t id) {
  dispatcher_.Subscribe(static_cast<events::EventListener*>(listener), id);
}

void Sdl2Application::UnSubscribe(common::IListener* listener, common::events_size_t id) {
  dispatcher_.UnSubscribe(static_cast<events::EventListener*>(listener), id);
}

void Sdl2Application::PostEvent(common::IEvent* event) {
  SDL_Event sevent;
  sevent.type = FASTO_EVENT;
  sevent.user.data1 = event;
  SDL_PushEvent(&sevent);
}

void Sdl2Application::Exit(int result) {
  stop_ = true;
}

void Sdl2Application::HandleEvent(events::Event* event) {
  dispatcher_.ProcessEvent(event);
}

namespace {
Keysym SDLKeySymToOur(SDL_Keysym sks) {
  Keysym ks;
  ks.mod = sks.mod;
  ks.scancode = static_cast<Scancode>(sks.scancode);
  ks.sym = sks.sym;
  return ks;
}
}

void Sdl2Application::HandleKeyPressEvent(SDL_KeyboardEvent* event) {
  if (event->type == SDL_KEYDOWN) {
    Keysym ks = SDLKeySymToOur(event->keysym);
    events::KeyPressInfo inf(event->state == SDL_PRESSED, ks);
    events::KeyPressEvent* key_press = new events::KeyPressEvent(this, inf);
    dispatcher_.ProcessEvent(key_press);
  } else if (event->type == SDL_KEYUP) {
    Keysym ks = SDLKeySymToOur(event->keysym);
    events::KeyReleaseInfo inf(event->state == SDL_PRESSED, ks);
    events::KeyReleaseEvent* key_release = new events::KeyReleaseEvent(this, inf);
    dispatcher_.ProcessEvent(key_release);
  }
}

void Sdl2Application::HandleWindowEvent(SDL_WindowEvent* event) {
  if (event->event == SDL_WINDOWEVENT_RESIZED) {
    events::WindowResizeInfo inf(event->data1, event->data2);
    events::WindowResizeEvent* wind_resize = new events::WindowResizeEvent(this, inf);
    dispatcher_.ProcessEvent(wind_resize);
  } else if (event->event == SDL_WINDOWEVENT_RESIZED) {
    events::WindowExposeInfo inf;
    events::WindowExposeEvent* wind_exp = new events::WindowExposeEvent(this, inf);
    dispatcher_.ProcessEvent(wind_exp);
  }
}

void Sdl2Application::HandleMousePressEvent(SDL_MouseButtonEvent* event) {}

void Sdl2Application::HandleMouseMoveEvent(SDL_MouseMotionEvent* event) {}
}
}
