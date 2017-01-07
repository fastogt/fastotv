#ifndef STREAM_HELPER_H
#define STREAM_HELPER_H

#include "ffmpeg_config.h"

#include "core/types.h"

static int exit_on_keydown;
static int64_t cursor_last_shown;
static int exit_on_mousedown;
static int cursor_hidden = 0;
static SDL_Renderer* renderer;
static SDL_Window* window;

VideoState* stream_open(const char* filename, AVInputFormat* iformat, AppOptions opt);
int stream_component_open(VideoState* is, int stream_index);
void stream_component_close(VideoState* is, int stream_index);

void step_to_next_frame(VideoState* is);
void stream_seek(VideoState* is, int64_t pos, int64_t rel, int seek_by_bytes);
void stream_close(VideoState* is);

void event_loop(VideoState* cur_stream);
void do_exit(VideoState* is);

int get_master_sync_type(VideoState* is);
double compute_target_delay(double delay, VideoState* is);
double get_master_clock(VideoState* is);

#endif
