#pragma once

#include "ffmpeg_config.h"

extern "C" {
#include <libavcodec/avcodec.h>

#include <SDL2/SDL.h>
}

#include "core/video_state.h"

#define FF_ALLOC_EVENT (SDL_USEREVENT)
#define FF_QUIT_EVENT (SDL_USEREVENT + 2)

int read_thread(void* user_data);
int video_thread(void* user_data);
int audio_thread(void* user_data);
int subtitle_thread(void* user_data);
