#include "core/application/sdl2_application.h"

#include <SDL2/SDL.h>         // for SDL_INIT_AUDIO, etc
#include <SDL2/SDL_error.h>   // for SDL_GetError
#include <SDL2/SDL_mutex.h>   // for SDL_CreateMutex, etc
#include <SDL2/SDL_stdinc.h>  // for SDL_getenv, SDL_setenv, etc

#include <common/threads/event_bus.h>

#include "core/events/events.h"

#include "network_controller.h"

#define FASTO_EVENT (SDL_USEREVENT)

namespace {
Keysym SDLKeySymToOur(SDL_Keysym sks) {
  Keysym ks;
  ks.mod = sks.mod;
  ks.scancode = static_cast<Scancode>(sks.scancode);
  ks.sym = sks.sym;
  return ks;
}
}

namespace fasto {
namespace fastotv {
namespace core {
namespace application {

Sdl2Application::Sdl2Application(int argc, char** argv)
    : common::application::IApplicationImpl(argc, argv),
      dispatcher_(),
      stop_(false),
      controller_(nullptr) {
  controller_ = new network::NetworkController(argc, argv);
}

Sdl2Application::~Sdl2Application() {
  destroy(&controller_);
}

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
  controller_->Start();
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
      case SDL_MOUSEBUTTONUP: {
        HandleMouseReleaseEvent(&event.button);
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
      case SDL_QUIT: {
        HandleQuitEvent(&event.quit);
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

void Sdl2Application::UnSubscribe(common::IListener* listener) {
  dispatcher_.UnSubscribe(static_cast<events::EventListener*>(listener));
}

void Sdl2Application::SendEvent(common::IEvent* event) {
  if (THREAD_MANAGER()->IsMainThread()) {
    PostEvent(event);
  } else {
    events::Event* fevent = static_cast<events::Event*>(event);
    HandleEvent(fevent);
  }
}

void Sdl2Application::PostEvent(common::IEvent* event) {
  SDL_Event sevent;
  sevent.type = FASTO_EVENT;
  sevent.user.data1 = event;
  SDL_PushEvent(&sevent);
}

void Sdl2Application::Exit(int result) {
  UNUSED(result);
  stop_ = true;
}

void Sdl2Application::ShowCursor() {
  SDL_ShowCursor(1);
}

void Sdl2Application::HideCursor() {
  SDL_ShowCursor(0);
}

void Sdl2Application::HandleEvent(events::Event* event) {
  dispatcher_.ProcessEvent(event);
}

void Sdl2Application::HandleKeyPressEvent(SDL_KeyboardEvent* event) {
  if (event->type == SDL_KEYDOWN) {
    Keysym ks = SDLKeySymToOur(event->keysym);
    events::KeyPressInfo inf(event->state == SDL_PRESSED, ks);
    events::KeyPressEvent* key_press = new events::KeyPressEvent(this, inf);
    HandleEvent(key_press);
  } else if (event->type == SDL_KEYUP) {
    Keysym ks = SDLKeySymToOur(event->keysym);
    events::KeyReleaseInfo inf(event->state == SDL_PRESSED, ks);
    events::KeyReleaseEvent* key_release = new events::KeyReleaseEvent(this, inf);
    HandleEvent(key_release);
  }
}

void Sdl2Application::HandleWindowEvent(SDL_WindowEvent* event) {  // SDL_WindowEventID
  if (event->event == SDL_WINDOWEVENT_RESIZED) {
    events::WindowResizeInfo inf(event->data1, event->data2);
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
}
}
}
}
