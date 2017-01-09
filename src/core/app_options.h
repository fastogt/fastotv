#pragma once

#include "ffmpeg_config.h"

extern "C" {
#include <libavutil/dict.h>
#include <libavutil/avutil.h>
}

#include <common/macros.h>

#include "core/types.h"

struct AppOptions {
  AppOptions();
  ~AppOptions();

#if CONFIG_AVFILTER
  void initAvFilters(const char* arg);
#endif

  int autorotate;
  int exit_on_keydown;
  int exit_on_mousedown;

  int audio_disable;
  int video_disable;
  int subtitle_disable;

  int64_t start_time;
  int64_t duration;
  int default_width;
  int default_height;
  int screen_width;
  int screen_height;

  ShowMode show_mode;  //
  const char* window_title;
  int framedrop;
  int genpts;
  AvSyncType av_sync_type;
  int startup_volume;
  int seek_by_bytes;
  int display_disable;
  int is_full_screen;
  int64_t audio_callback_time;
  const char* input_filename;  //
  int loop;
  int autoexit;
  int show_status;
  int infinite_buffer;
  const char* wanted_stream_spec[AVMEDIA_TYPE_NB];
  double rdftspeed;
  int lowres;

  /* options specified by the user */
  int fast;
  const char* audio_codec_name;
  const char* subtitle_codec_name;
  const char* video_codec_name;

  int decoder_reorder_pts;

#if CONFIG_AVFILTER
  const char** vfilters_list;
  int nb_vfilters;
  char* afilters;
#endif

  AVDictionary* sws_dict;
  AVDictionary* swr_opts;
  AVDictionary* format_opts;
  AVDictionary* codec_opts;

 private:
  DISALLOW_COPY_AND_ASSIGN(AppOptions);
};
