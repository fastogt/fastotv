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

#include "client/sdl_utils.h"

#include <setjmp.h>  // for longjmp, jmp_buf, setjmp
#include <stdio.h>   // for NULL, fclose, fopen, FILE
#include <string>    // for string

#include <SDL2/SDL_endian.h>  // for SDL_BYTEORDER, SDL_LIL_ENDIAN
#include <SDL2/SDL_pixels.h>  // for SDL_Color, SDL_Palette, SDL_PixelFormat

#include <common/macros.h>  // for SIZEOFMASS, PROJECT_VERSION_CHECK, PROJ...
#include <common/value.h>   // for Value, Value::ErrorsType::E_ERROR

namespace fasto {
namespace fastotv {
namespace client {
SurfaceSaver::SurfaceSaver(SDL_Surface* surface) : surface_(surface), texture_(NULL), renderer_(NULL) {}

SurfaceSaver::~SurfaceSaver() {
  if (renderer_) {
    renderer_ = NULL;
  }

  if (texture_) {
    SDL_DestroyTexture(texture_);
    texture_ = NULL;
  }

  if (surface_) {
    SDL_FreeSurface(surface_);
    surface_ = NULL;
  }
}

SDL_Texture* SurfaceSaver::GetTexture(SDL_Renderer* renderer) const {
  if (!renderer || !surface_) {
    return NULL;
  }

  if (!texture_ || renderer_ != renderer) {
    if (texture_) {
      SDL_DestroyTexture(texture_);
      texture_ = NULL;
    }

    texture_ = SDL_CreateTextureFromSurface(renderer_, surface_);
    renderer_ = renderer;
  }

  return texture_;
}

int SurfaceSaver::GetWidthSurface() const {
  if (!surface_) {
    return 0;
  }

  return surface_->w;
}

int SurfaceSaver::GetHeightSurface() const {
  if (!surface_) {
    return 0;
  }

  return surface_->h;
}

TextureSaver::TextureSaver() : texture_(NULL), renderer_(NULL) {}

int TextureSaver::GetWidth() const {
  Uint32 format;
  int access, w, h;
  if (SDL_QueryTexture(texture_, &format, &access, &w, &h) < 0) {
    return 0;
  }

  return w;
}

int TextureSaver::GetHeight() const {
  Uint32 format;
  int access, w, h;
  if (SDL_QueryTexture(texture_, &format, &access, &w, &h) < 0) {
    return 0;
  }

  return h;
}

SDL_Texture* TextureSaver::GetTexture(SDL_Renderer* renderer, int width, int height, Uint32 format) const {
  if (!renderer) {
    return NULL;
  }

  Uint32 cur_format;
  int access, w, h;
  if (renderer_ != renderer || SDL_QueryTexture(texture_, &cur_format, &access, &w, &h) < 0 || width != w ||
      height != h || format != cur_format) {
    SDL_DestroyTexture(texture_);
    texture_ = NULL;
    renderer_ = NULL;

    SDL_Texture* ltexture = NULL;
    common::Error err = CreateTexture(renderer, format, width, height, SDL_BLENDMODE_NONE, false, &ltexture);
    if (err && err->IsError()) {
      DNOTREACHED() << err->Description();
      return NULL;
    }
    texture_ = ltexture;
    renderer_ = renderer;
  }

  return texture_;
}

TextureSaver::~TextureSaver() {
  if (texture_) {
    SDL_DestroyTexture(texture_);
    texture_ = NULL;
  }
  renderer_ = NULL;
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
    return common::make_error_value("Couldn't allocate memory for texture", common::Value::E_ERROR);
  }
  if (SDL_SetTextureBlendMode(ltexture, blendmode) < 0) {
    SDL_DestroyTexture(ltexture);
    return common::make_error_value("Couldn't set blend mode for texture", common::Value::E_ERROR);
  }
  if (init_texture) {
    void* pixels;
    int pitch;
    if (SDL_LockTexture(ltexture, NULL, &pixels, &pitch) < 0) {
      SDL_DestroyTexture(ltexture);
      return common::make_error_value("Couldn't lock texture", common::Value::E_ERROR);
    }
    const size_t pixels_size = pitch * new_height;
    memset(pixels, 0, pixels_size);
    SDL_UnlockTexture(ltexture);
  }

  *texture_out = ltexture;
  return common::Error();
}

SDL_Rect GetCenterRect(SDL_Rect rect, int width, int height) {
  int calc_width = rect.w;
  int calc_height = rect.h;
  int calc_x = rect.x;
  int calc_y = rect.y;

  if (rect.w >= width) {
    calc_x = rect.x + rect.w / 2 - width / 2;
    calc_width = width;
  }
  if (rect.h >= height) {
    calc_y = rect.y + rect.h / 2 - height / 2;
    calc_height = height;
  }

  return {calc_x, calc_y, calc_width, calc_height};
}

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
