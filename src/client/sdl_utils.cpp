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

#include "sdl_utils.h"

extern "C" {
#include <png.h>
}

#include <common/macros.h>

SDL_Texture* CreateTexture(SDL_Renderer* renderer,
                           Uint32 new_format,
                           int new_width,
                           int new_height,
                           SDL_BlendMode blendmode,
                           bool init_texture) {
  SDL_Texture* ltexture =
      SDL_CreateTexture(renderer, new_format, SDL_TEXTUREACCESS_STREAMING, new_width, new_height);
  if (!ltexture) {
    NOTREACHED();
    return nullptr;
  }
  if (SDL_SetTextureBlendMode(ltexture, blendmode) < 0) {
    NOTREACHED();
    SDL_DestroyTexture(ltexture);
    return nullptr;
  }
  if (init_texture) {
    void* pixels;
    int pitch;
    if (SDL_LockTexture(ltexture, NULL, &pixels, &pitch) < 0) {
      NOTREACHED();
      SDL_DestroyTexture(ltexture);
      return nullptr;
    }
    const size_t pixels_size = pitch * new_height;
    memset(pixels, 0, pixels_size);
    SDL_UnlockTexture(ltexture);
  }

  return ltexture;
}

SDL_Surface* IMG_LoadPNG(const char* path) {
  unsigned char header[8];  // 8 is the maximum size that can be checked
  FILE* fp = fopen(path, "rb");
  if (!fp) {
    WARNING_LOG() << "Couldn't open png file path: " << path;
    return NULL;
  }

  size_t res = fread(header, sizeof(unsigned char), SIZEOFMASS(header), fp);
  if (res != SIZEOFMASS(header) || png_sig_cmp(header, 0, SIZEOFMASS(header))) {
    WARNING_LOG() << "Invalid png header path: " << path;
    fclose(fp);
    return NULL;
  }

  /* Create the PNG loading context structure */
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL) {
    WARNING_LOG() << "Couldn't allocate memory for PNG file or incompatible PNG dll";
    fclose(fp);
    return NULL;
  }

  /* Allocate/initialize the memory for image information.  REQUIRED. */
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    WARNING_LOG() << "Couldn't create image information for PNG file";
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    fclose(fp);
    return NULL;
  }

/* Set error handling if you are using setjmp/longjmp method (this is
 * the normal method of doing things with libpng).  REQUIRED unless you
 * set up your own error handlers in png_create_read_struct() earlier.
 */
#if PROJECT_VERSION_CHECK(PNG_LIBPNG_VER_MAJOR, PNG_LIBPNG_VER_MINOR, PNG_LIBPNG_VER_RELEASE) >= \
    PROJECT_VERSION_GENERATE(1, 5, 0)
  if (setjmp(*png_set_longjmp_fn(png_ptr, longjmp, sizeof(jmp_buf)))) {
#else
  if (setjmp(png_ptr->jmpbuf)) {
#endif
    WARNING_LOG() << "Error reading the PNG file.";
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return NULL;
  }

  /* Set up the input control */
  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;
  SDL_Palette* palette;
  int ckey = -1;
  png_color_16* transv;
  /* Read PNG header info */
  png_read_info(png_ptr, info_ptr);
  png_get_IHDR(
      png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

  /* tell libpng to strip 16 bit/color files down to 8 bits/color */
  png_set_strip_16(png_ptr);

  /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
   * byte into separate bytes (useful for paletted and grayscale images).
   */
  png_set_packing(png_ptr);

  /* scale greyscale values to the range 0..255 */
  if (color_type == PNG_COLOR_TYPE_GRAY)
    png_set_expand(png_ptr);

  /* For images with a single "transparent colour", set colour key;
     if more than one index has transparency, or if partially transparent
     entries exist, use full alpha channel */
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    int num_trans;
    Uint8* trans;
    png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &transv);
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
      /* Check if all tRNS entries are opaque except one */
      int j, t = -1;
      for (j = 0; j < num_trans; j++) {
        if (trans[j] == 0) {
          if (t >= 0) {
            break;
          }
          t = j;
        } else if (trans[j] != 255) {
          break;
        }
      }
      if (j == num_trans) {
        /* exactly one transparent index */
        ckey = t;
      } else {
        /* more than one transparent index, or translucency */
        png_set_expand(png_ptr);
      }
    } else {
      ckey = 0; /* actual value will be set later */
    }
  }

  if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    png_set_gray_to_rgb(png_ptr);
  }

  png_read_update_info(png_ptr, info_ptr);

  png_get_IHDR(
      png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

  /* Allocate the SDL surface to hold the image */
  Uint32 Rmask = 0;
  Uint32 Gmask = 0;
  Uint32 Bmask = 0;
  Uint32 Amask = 0;
  int num_channels = png_get_channels(png_ptr, info_ptr);
  if (num_channels >= 3) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    Rmask = 0x000000FF;
    Gmask = 0x0000FF00;
    Bmask = 0x00FF0000;
    Amask = (num_channels == 4) ? 0xFF000000 : 0;
#else
    int s = (num_channels == 4) ? 0 : 8;
    Rmask = 0xFF000000 >> s;
    Gmask = 0x00FF0000 >> s;
    Bmask = 0x0000FF00 >> s;
    Amask = 0x000000FF >> s;
#endif
  }
  SDL_Surface* volatile surface = SDL_CreateRGBSurface(
      SDL_SWSURFACE, width, height, bit_depth * num_channels, Rmask, Gmask, Bmask, Amask);
  if (surface == NULL) {
    WARNING_LOG() << SDL_GetError();
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    fclose(fp);
    return NULL;
  }

  if (ckey != -1) {
    if (color_type != PNG_COLOR_TYPE_PALETTE) {
      /* FIXME: Should these be truncated or shifted down? */
      ckey = SDL_MapRGB(
          surface->format, (Uint8)transv->red, (Uint8)transv->green, (Uint8)transv->blue);
    }
    SDL_SetColorKey(surface, SDL_TRUE, ckey);
  }

  /* Create the array of pointers to image data */
  png_bytep* volatile row_pointers =
      static_cast<png_bytep*>(SDL_malloc(sizeof(png_bytep) * height));
  if (!row_pointers) {
    WARNING_LOG() << "Out of memory";
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    fclose(fp);
    return NULL;
  }
  for (png_uint_32 row = 0; row < height; row++) {
    row_pointers[row] = static_cast<png_bytep>(surface->pixels) + row * surface->pitch;
  }

  /* Read the entire image in one go */
  png_read_image(png_ptr, row_pointers);

  /* and we're done!  (png_read_end() can be omitted if no processing of
   * post-IDAT text/time/etc. is desired)
   * In some cases it can't read PNG's created by some popular programs (ACDSEE),
   * we do not want to process comments, so we omit png_read_end

  lib.png_read_end(png_ptr, info_ptr);
  */

  /* Load the palette, if any */
  palette = surface->format->palette;
  if (palette) {
    int png_num_palette;
    png_colorp png_palette;
    png_get_PLTE(png_ptr, info_ptr, &png_palette, &png_num_palette);
    if (color_type == PNG_COLOR_TYPE_GRAY) {
      palette->ncolors = 256;
      for (int i = 0; i < 256; i++) {
        palette->colors[i].r = i;
        palette->colors[i].g = i;
        palette->colors[i].b = i;
      }
    } else if (png_num_palette > 0) {
      palette->ncolors = png_num_palette;
      for (int i = 0; i < png_num_palette; ++i) {
        palette->colors[i].b = png_palette[i].blue;
        palette->colors[i].g = png_palette[i].green;
        palette->colors[i].r = png_palette[i].red;
      }
    }
  }

  SDL_free(row_pointers);
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  fclose(fp);
  return (surface);
}
