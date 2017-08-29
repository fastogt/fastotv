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

#include <SDL2/SDL_render.h>   // for SDL_Renderer, SDL_Texture
#include <SDL2/SDL_surface.h>  // for SDL_Surface

#include <common/macros.h>

namespace fastotv {
namespace client {
namespace player {
namespace draw {

class TextureSaver {
 public:
  TextureSaver();
  ~TextureSaver();

  SDL_Texture* GetTexture(SDL_Renderer* renderer, int width, int height, Uint32 format) const;

  int GetWidth() const;
  int GetHeight() const;

 private:
  mutable SDL_Texture* texture_;
  mutable SDL_Renderer* renderer_;
};

}  // namespace draw
}  // namespace player
}  // namespace client
}  // namespace fastotv
