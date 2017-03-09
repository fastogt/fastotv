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

namespace fasto {
namespace fastotv {
namespace core {

// 640x480
// 1280x720
struct AppOptions {
  AppOptions();

  bool autorotate;

  int seek_by_bytes;

  bool framedrop;
  bool genpts;
  AvSyncType av_sync_type;
  bool show_status;
  int infinite_buffer;
  std::string wanted_stream_spec[AVMEDIA_TYPE_NB];
  int lowres;

  /* options specified by the user */
  bool fast;
  std::string audio_codec_name;
  std::string video_codec_name;

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
}
}
