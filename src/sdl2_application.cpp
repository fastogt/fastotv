#include "sdl2_application.h"

#include <SDL2/SDL.h>         // for SDL_INIT_AUDIO, etc
#include <SDL2/SDL_error.h>   // for SDL_GetError
#include <SDL2/SDL_mutex.h>   // for SDL_CreateMutex, etc
#include <SDL2/SDL_stdinc.h>  // for SDL_getenv, SDL_setenv, etc

#include <common/threads/event_bus.h>

#define FF_ALLOC_EVENT (SDL_USEREVENT)
#define FF_QUIT_EVENT (SDL_USEREVENT + 2)
#define FF_NEXT_STREAM (SDL_USEREVENT + 3)
#define FF_PREV_STREAM (SDL_USEREVENT + 4)

#define FASTO_EVENT (SDL_USEREVENT)

Sdl2Application::Sdl2Application(int argc, char** argv)
    : common::application::IApplicationImpl(argc, argv), stop_(false) {}

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
      TimeInfo inf;
      TimerEvent timer_event(this, inf);
      HandleEvent(&timer_event);
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
        Event* fevent = static_cast<Event*>(event.user.data1);
        HandleEvent(fevent);
        delete fevent;
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

void Sdl2Application::PostEvent(Event* event) {
  SDL_Event sevent;
  sevent.type = FASTO_EVENT;
  sevent.user.data1 = event;
  SDL_PushEvent(&sevent);
}

void Sdl2Application::Exit(int result) {
  stop_ = true;
}

void Sdl2Application::HandleEvent(Event* event) {

}

void Sdl2Application::HandleKeyPressEvent(SDL_KeyboardEvent* event) {}

void Sdl2Application::HandleWindowEvent(SDL_WindowEvent* event) {}

void Sdl2Application::HandleMousePressEvent(SDL_MouseButtonEvent* event) {}

void Sdl2Application::HandleMouseMoveEvent(SDL_MouseMotionEvent* event) {}
