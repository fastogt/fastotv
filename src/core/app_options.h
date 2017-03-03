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

// 640x480
// 1280x720
struct AppOptions {
  enum { loop_count = 1 };
  AppOptions();

  bool autorotate;

  bool audio_disable;
  bool video_disable;

  int seek_by_bytes;
  int64_t start_time;
  int64_t duration;

  bool framedrop;
  bool genpts;
  AvSyncType av_sync_type;
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
  ComplexOptions(AVDictionary* sws_dict,
                 AVDictionary* swr_opts,
                 AVDictionary* format_opts,
                 AVDictionary* codec_opts);

  ~ComplexOptions();
  ComplexOptions(const ComplexOptions& other);
  ComplexOptions& operator=(const ComplexOptions& rhs);

  AVDictionary* sws_dict;
  AVDictionary* swr_opts;
  AVDictionary* format_opts;
  AVDictionary* codec_opts;
};

}  // namespace core
