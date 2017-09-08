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

#include "client/player/draw/draw.h"

#include <limits.h>

#include <SDL2/SDL_hints.h>

#include <common/sprintf.h>

namespace fastotv {
namespace client {
namespace player {
namespace draw {

common::Error CreateMainWindow(const common::draw::Size& size,
                               bool is_full_screen,
                               const std::string& title,
                               SDL_Renderer** renderer,
                               SDL_Window** window) {
  if (!renderer || !window || !size.IsValid()) {  // invalid input
    return common::make_error_inval();
  }

  Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
  if (is_full_screen) {
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
  }
  int num_video_drivers = SDL_GetNumVideoDrivers();
  for (int i = 0; i < num_video_drivers; ++i) {
    DEBUG_LOG() << "Available video driver: " << SDL_GetVideoDriver(i);
  }
  SDL_Window* lwindow =
      SDL_CreateWindow(NULL, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, size.width, size.height, flags);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  SDL_Renderer* lrenderer = NULL;
  if (lwindow) {
    DEBUG_LOG() << "Initialized video driver: " << SDL_GetCurrentVideoDriver();
    int n = SDL_GetNumRenderDrivers();
    for (int i = 0; i < n; ++i) {
      SDL_RendererInfo renderer_info;
      int res = SDL_GetRenderDriverInfo(i, &renderer_info);
      if (res == 0) {
        bool is_hardware_renderer = renderer_info.flags & SDL_RENDERER_ACCELERATED;
        std::string screen_size =
            common::MemSPrintf(" maximum texture size can be %dx%d.",
                               renderer_info.max_texture_width == 0 ? INT_MAX : renderer_info.max_texture_width,
                               renderer_info.max_texture_height == 0 ? INT_MAX : renderer_info.max_texture_height);
        DEBUG_LOG() << "Available renderer: " << renderer_info.name
                    << (is_hardware_renderer ? "(hardware)" : "(software)") << screen_size;
      }
    }
    lrenderer = SDL_CreateRenderer(lwindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (lrenderer) {
      SDL_RendererInfo info;
      if (SDL_GetRendererInfo(lrenderer, &info) == 0) {
        bool is_hardware_renderer = info.flags & SDL_RENDERER_ACCELERATED;
        DEBUG_LOG() << "Initialized renderer: " << info.name << (is_hardware_renderer ? " (hardware)." : "(software).");
      }
    } else {
      WARNING_LOG() << "Failed to initialize a hardware accelerated renderer: " << SDL_GetError();
      lrenderer = SDL_CreateRenderer(lwindow, -1, 0);
    }
  }

  if (!lwindow || !lrenderer) {
    ERROR_LOG() << "SDL: could not set video mode - exiting";
    if (lrenderer) {
      SDL_DestroyRenderer(lrenderer);
    }
    if (lwindow) {
      SDL_DestroyWindow(lwindow);
    }
    return common::make_error("Could not set video mode");
  }

  SDL_SetRenderDrawBlendMode(lrenderer, SDL_BLENDMODE_BLEND);
  SDL_SetWindowSize(lwindow, size.width, size.height);
  SDL_SetWindowTitle(lwindow, title.c_str());

  *window = lwindow;
  *renderer = lrenderer;
  return common::Error();
}

common::Error CreateTexture(SDL_Renderer* renderer,
                            Uint32 new_format,
                            int new_width,
                            int new_height,
                            SDL_BlendMode blendmode,
                            bool init_texture,
                            SDL_Texture** texture_out) {
  SDL_Texture* ltexture = SDL_CreateTexture(renderer, new_format, SDL_TEXTUREACCESS_STREAMING, new_width, new_height);
  if (!ltexture) {
    return common::make_error("Couldn't allocate memory for texture");
  }
  if (SDL_SetTextureBlendMode(ltexture, blendmode) < 0) {
    SDL_DestroyTexture(ltexture);
    return common::make_error("Couldn't set blend mode for texture");
  }
  if (init_texture) {
    void* pixels;
    int pitch;
    if (SDL_LockTexture(ltexture, NULL, &pixels, &pitch) < 0) {
      SDL_DestroyTexture(ltexture);
      return common::make_error("Couldn't lock texture");
    }
    const size_t pixels_size = pitch * new_height;
    memset(pixels, 0, pixels_size);
    SDL_UnlockTexture(ltexture);
  }

  *texture_out = ltexture;
  return common::Error();
}

common::Error SetRenderDrawColor(SDL_Renderer* render, const SDL_Color& rgba) {
  if (!render) {
    return common::make_error_inval();
  }

  int res = SDL_SetRenderDrawColor(render, rgba.r, rgba.g, rgba.b, rgba.a);
  if (res == -1) {
    return common::make_error("Couldn't set draw color for render.");
  }

  return common::Error();
}

common::Error FillRectColor(SDL_Renderer* render, const SDL_Rect& rect, const SDL_Color& rgba) {
  common::Error err = SetRenderDrawColor(render, rgba);
  if (err) {
    return err;
  }

  int res = SDL_RenderFillRect(render, &rect);
  if (res == -1) {
    return common::make_error("Couldn't fill rect.");
  }
  return common::Error();
}

common::Error DrawBorder(SDL_Renderer* render, const SDL_Rect& rect, const SDL_Color& rgba) {
  common::Error err = SetRenderDrawColor(render, rgba);
  if (err) {
    return err;
  }

  int res = SDL_RenderDrawRect(render, &rect);
  if (res == -1) {
    return common::make_error("Couldn't draw rect.");
  }
  return common::Error();
}

common::Error FlushRender(SDL_Renderer* render, const SDL_Color& rgba) {
  common::Error err = SetRenderDrawColor(render, rgba);
  if (err) {
    return err;
  }

  int res = SDL_RenderClear(render);
  if (res == -1) {
    return common::make_error("Couldn't clear render.");
  }
  return common::Error();
}

}  // namespace draw
}  // namespace player
}  // namespace client
}  // namespace fastotv
