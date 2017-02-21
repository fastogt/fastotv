#pragma once

#include <stdint.h>  // for int64_t
#include <string>
#include <vector>

#include "ffmpeg_config.h"

extern "C" {
#include <libavutil/dict.h>
#include <libavutil/avutil.h>
}

#include <common/macros.h>

#include "core/types.h"

namespace core {

struct AppOptions {
  enum { width = 640, height = 480, volume = 100, loop_count = 1 };
  AppOptions();

  bool autorotate;

  bool audio_disable;
  bool video_disable;

  int seek_by_bytes;
  int64_t start_time;
  int64_t duration;
  int default_width;
  int default_height;
  int screen_width;
  int screen_height;

  ShowMode show_mode;
  int framedrop;
  bool genpts;
  AvSyncType av_sync_type;
  int startup_volume;
  bool display_disable;
  int loop;
  bool autoexit;
  bool show_status;
  int infinite_buffer;
  std::string wanted_stream_spec[AVMEDIA_TYPE_NB];
  int lowres;

  /* options specified by the user */
  bool fast;
  std::string audio_codec_name;
  std::string video_codec_name;

  int decoder_reorder_pts;

#if CONFIG_AVFILTER
  void InitAvFilters(const std::string& arg);
  std::vector<std::string> vfilters_list;
  std::string afilters;
#endif
};

struct ComplexOptions {
  ComplexOptions();

  AVDictionary* sws_dict;
  AVDictionary* swr_opts;
  AVDictionary* format_opts;
  AVDictionary* codec_opts;

 private:
  DISALLOW_COPY_AND_ASSIGN(ComplexOptions);
};

}  // namespace core
