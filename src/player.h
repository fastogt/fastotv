#pragma once

#include <SDL2/SDL_events.h>  // for SDL_Event, SDL_PushEvent, etc

#include "video_state_handler.h"

#include "url.h"

#include <common/smart_ptr.h>
#include <common/url.h>

namespace core {
struct AppOptions;
}
namespace core {
struct ComplexOptions;
}

class VideoState;

struct PlayerOptions {
  PlayerOptions();

  common::uri::Uri play_list_location;
  bool exit_on_keydown;
  bool exit_on_mousedown;
  bool is_full_screen;
};

class Player : public VideoStateHandler {
 public:
  Player(const PlayerOptions& options, core::AppOptions* opt, core::ComplexOptions* copt);
  int Exec() WARN_UNUSED_RESULT;
  void Stop();
  void SetFullScreen(bool full_screen);
  ~Player();

 protected:
  virtual bool RequestWindow(VideoState* stream,
                             int width,
                             int height,
                             SDL_Renderer** renderer,
                             SDL_Window** window) override;

  virtual void HandleKeyPressEvent(SDL_KeyboardEvent* event);
  virtual void HandleWindowEvent(SDL_WindowEvent* event);
  virtual void HandleMousePressEvent(SDL_MouseButtonEvent* event);
  virtual void HandleMouseMoveEvent(SDL_MouseMotionEvent* event);

 private:
  Url CurrentUrl() const;
  void SwitchToErrorMode();

  bool ChangePlayListLocation(const common::uri::Uri& location);
  VideoState* CreateCurrentStream();
  VideoState* CreateNextStream();
  VideoState* CreatePrevStream();

  PlayerOptions options_;
  core::AppOptions* opt_;
  core::ComplexOptions* copt_;
  std::vector<Url> play_list_;

  SDL_Renderer* renderer_;
  SDL_Window* window_;
  bool cursor_hidden_;
  int64_t cursor_last_shown_;
  int64_t last_mouse_left_click_;
  size_t curent_stream_pos_;

  bool stop_;
  VideoState* stream_;
};
