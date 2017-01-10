#pragma once

extern "C" {
#include <SDL2/SDL.h>
}

#define FF_ALLOC_EVENT (SDL_USEREVENT)
#define FF_QUIT_EVENT (SDL_USEREVENT + 2)

int read_thread(void* user_data);
int video_thread(void* user_data);
int audio_thread(void* user_data);
int subtitle_thread(void* user_data);
