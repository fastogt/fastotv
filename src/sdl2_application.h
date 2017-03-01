#pragma once

#include <SDL2/SDL_events.h>  // for SDL_EventState, SDL_IGNORE, etc

#include <common/application/application.h>

#include "events.h"

class Sdl2Application : public common::application::IApplicationImpl<Event> {
 public:
  enum { event_timeout_wait_msec = 10 };
  Sdl2Application(int argc, char** argv);

  virtual int PreExec() override;   // EXIT_FAILURE, EXIT_SUCCESS
  virtual int Exec() override;      // EXIT_FAILURE, EXIT_SUCCESS
  virtual int PostExec() override;  // EXIT_FAILURE, EXIT_SUCCESS

  virtual void PostEvent(Event* event) override;

  virtual void Exit(int result) override;

 protected:
  virtual void HandleEvent(Event* event);

  virtual void HandleKeyPressEvent(SDL_KeyboardEvent* event);
  virtual void HandleWindowEvent(SDL_WindowEvent* event);
  virtual void HandleMousePressEvent(SDL_MouseButtonEvent* event);
  virtual void HandleMouseMoveEvent(SDL_MouseMotionEvent* event);

 private:
  bool stop_;
};
