#pragma once

#include "ffmpeg_config.h"

extern "C" {
#include <libavcodec/avcodec.h>

#include <SDL2/SDL.h>
}

#include "core/video_state.h"

#define FF_ALLOC_EVENT (SDL_USEREVENT)
#define FF_QUIT_EVENT (SDL_USEREVENT + 2)

#if CONFIG_AVFILTER
static const char** vfilters_list = NULL;
static int nb_vfilters = 0;
static char* afilters = NULL;
#endif

static int framedrop = -1;
static int genpts = 0;
static int av_sync_type = AV_SYNC_AUDIO_MASTER;
static int startup_volume = 100;
static int seek_by_bytes = -1;
static const char* input_filename;
static int loop = 1;
static int autoexit;
static int show_status = 1;
static int infinite_buffer = -1;
static const char* wanted_stream_spec[AVMEDIA_TYPE_NB] = {0};
static int display_disable;
static double rdftspeed = 0.02;
static int lowres = 0;

/* options specified by the user */
static AVInputFormat* file_iformat;
static int fast = 0;
static const char* audio_codec_name;
static const char* subtitle_codec_name;
static const char* video_codec_name;

/* current context */
static int is_full_screen;
static int64_t audio_callback_time;

int read_thread(void* user_data);
int video_thread(void* user_data);
int audio_thread(void* user_data);
int subtitle_thread(void* user_data);
