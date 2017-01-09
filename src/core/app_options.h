#pragma once

extern "C" {
#include <libavutil/dict.h>
}

#include "core/types.h"

struct AppOptions {
  AppOptions();

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

  ShowMode show_mode; //
  const char* window_title;

  AVDictionary* sws_dict;
  AVDictionary* swr_opts;
  AVDictionary* format_opts;
  AVDictionary* codec_opts;
};
