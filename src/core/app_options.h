#pragma once

extern "C" {
#include <libavutil/dict.h>
}

struct AppOptions {
  AppOptions();

  int autorotate;
  int exit_on_keydown;
  int exit_on_mousedown;

  AVDictionary* sws_dict;
  AVDictionary* swr_opts;
  AVDictionary* format_opts;
  AVDictionary* codec_opts;
};
