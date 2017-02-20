#pragma once

#include "video_state_handler.h"

#include <common/smart_ptr.h>
#include <common/url.h>

namespace core {
struct AppOptions;
}
namespace core {
struct ComplexOptions;
}

class VideoState;

class Player : public VideoStateHandler {
 public:
  Player(const common::uri::Uri& play_list_location,
         core::AppOptions* opt,
         core::ComplexOptions* copt);
  int Exec() WARN_UNUSED_RESULT;
  ~Player();

 protected:
  virtual bool RequestWindow(const common::uri::Uri& uri,
                             int width,
                             int height,
                             SDL_Renderer** renderer, SDL_Window** window) override;

 private:
  bool ChangePlayListLocation(const common::uri::Uri& location);

  core::AppOptions* opt_;
  core::ComplexOptions* copt_;
  std::vector<common::uri::Uri> play_list_;
  common::scoped_ptr<VideoState> stream_;

  SDL_Renderer* renderer_;
  SDL_Window* window_;
  bool cursor_hidden_;
  int64_t cursor_last_shown_;
};
