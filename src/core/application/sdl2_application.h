#pragma once

#include <SDL2/SDL_events.h>  // for SDL_EventState, SDL_IGNORE, etc

#include <common/application/application.h>
#include <common/threads/event_dispatcher.h>

#include "core/events/events_base.h"

namespace fasto {
namespace fastotv {
namespace network {
class NetworkController;
}
namespace core {
namespace application {

class Sdl2Application : public common::application::IApplicationImpl {
 public:
  enum { event_timeout_wait_msec = 10 };
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
  network::NetworkController* controller_;
};
}
}
}
}
