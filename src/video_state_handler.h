#pragma once

#include <SDL2/SDL_render.h>  // for SDL_Renderer, SDL_Texture
#include "ffmpeg_config.h"

extern "C" {
#include <libavformat/avformat.h>  // for av_find_input_format, etc
}

class VideoState;
class VideoStateHandler {
 public:
  VideoStateHandler();
  virtual ~VideoStateHandler();

  virtual bool RequestWindow(VideoState* stream,
                             int width,
                             int height,
                             SDL_Renderer** renderer,
                             SDL_Window** window) = 0;
};
