/*  Copyright (C) 2014-2017 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

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

#include "ffmpeg_internal.h"
#include "client/core/types.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

// 640x480
// 1280x720
// 1920x1080

enum FRAME_DROP_STRATEGY { FRAME_DROP_AUTO = -1, FRAME_DROP_OFF = 0, FRAME_DROP_ON = 1 };

struct AppOptions {
  AppOptions();
  bool autorotate;

  FRAME_DROP_STRATEGY framedrop;
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
  enum HWAccelID hwaccel_id;
  std::string hwaccel_device;
  std::string hwaccel_output_format;

  bool auto_exit;
#if CONFIG_AVFILTER
  std::string vfilters;
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
}
