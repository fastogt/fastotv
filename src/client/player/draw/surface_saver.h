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

class SurfaceSaver {
 public:
  explicit SurfaceSaver(SDL_Surface* surface);
  ~SurfaceSaver();

  SDL_Texture* GetTexture(SDL_Renderer* renderer) const;

  int GetWidthSurface() const;
  int GetHeightSurface() const;

 private:
  DISALLOW_COPY_AND_ASSIGN(SurfaceSaver);
  SDL_Surface* surface_;
  mutable SDL_Texture* texture_;
  mutable SDL_Renderer* renderer_;
};

SurfaceSaver* MakeSurfaceFromPath(const std::string& img_full_path);

}  // namespace draw
}  // namespace player
}  // namespace client
}  // namespace fastotv
