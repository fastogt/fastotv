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
static const char* window_title;
static enum ShowMode show_mode = SHOW_MODE_NONE;
static int autoexit;
static int show_status = 1;
static int audio_disable;
static int video_disable;
static int subtitle_disable;
static int infinite_buffer = -1;
static int64_t start_time = AV_NOPTS_VALUE;
static const char* wanted_stream_spec[AVMEDIA_TYPE_NB] = {0};
static int64_t duration = AV_NOPTS_VALUE;
static int default_width = 640;
static int default_height = 480;
static int screen_width = 0;
static int screen_height = 0;
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

void calculate_display_rect(SDL_Rect* rect,
                            int scr_xleft,
                            int scr_ytop,
                            int scr_width,
                            int scr_height,
                            int pic_width,
                            int pic_height,
                            AVRational pic_sar);
void set_default_window_size(int width, int height, AVRational sar);

int read_thread(void* user_data);
int video_thread(void* user_data);
int audio_thread(void* user_data);
int subtitle_thread(void* user_data);
