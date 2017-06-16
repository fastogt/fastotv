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

#pragma once

#include <SDL2/SDL_blendmode.h>  // for SDL_BlendMode
#include <SDL2/SDL_render.h>     // for SDL_Renderer, SDL_Texture
#include <SDL2/SDL_stdinc.h>     // for Uint32
#include <SDL2/SDL_surface.h>    // for SDL_Surface

#include <common/error.h>  // for Error

namespace fasto {
namespace fastotv {
namespace client {

class TextureSaver {
 public:
  explicit TextureSaver(SDL_Surface* surface);
  ~TextureSaver();
  SDL_Texture* GetTexture(SDL_Renderer* renderer) const;

  int GetWidthSurface() const;
  int GetHeightSurface() const;

 private:
  DISALLOW_COPY_AND_ASSIGN(TextureSaver);
  SDL_Surface* surface_;
  mutable SDL_Texture* texture_;
  mutable SDL_Renderer* renderer_;
};

common::Error CreateTexture(SDL_Renderer* renderer,
                            Uint32 new_format,
                            int new_width,
                            int new_height,
                            SDL_BlendMode blendmode,
                            bool init_texture,
                            SDL_Texture** texture_out);

SDL_Rect GetCenterRect(SDL_Rect rect, int width, int height);

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
