#ifndef CORE_THREADS_H
#define CORE_THREADS_H

#include <libavcodec/avcodec.h>

#include <SDL2/SDL.h>

#include "config.h"

#define FF_ALLOC_EVENT (SDL_USEREVENT)
#define FF_QUIT_EVENT (SDL_USEREVENT + 2)

#if CONFIG_AVFILTER
static const char** vfilters_list = NULL;
static int nb_vfilters = 0;
static char* afilters = NULL;
#endif

static int decoder_reorder_pts = -1;
static int framedrop = -1;
static AVPacket flush_pkt;

int video_thread(void* arg);
int audio_thread(void* arg);
int subtitle_thread(void* arg);

#endif
