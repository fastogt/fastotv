#pragma once

#include <SDL2/SDL_render.h>

SDL_Texture* CreateTexture(SDL_Renderer* renderer,
                           Uint32 new_format,
                           int new_width,
                           int new_height,
                           SDL_BlendMode blendmode,
                           bool init_texture);
SDL_Surface* IMG_LoadPNG(const char* path);
