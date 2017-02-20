#pragma once

#include <SDL2/SDL_render.h>  // for SDL_Renderer, SDL_Texture

#include <common/url.h>

class VideoStateHandler {
 public:
  VideoStateHandler();
  virtual ~VideoStateHandler();

  virtual bool RequestWindow(const common::uri::Uri& uri,
                             int width,
                             int height,
                             SDL_Renderer** renderer, SDL_Window** window) = 0;
};
