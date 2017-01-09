#include "core/app_options.h"

#include <stddef.h>

#include <libavutil/avutil.h>

AppOptions::AppOptions()
    : autorotate(0),
      exit_on_keydown(0),
      exit_on_mousedown(0),
      audio_disable(0),
      video_disable(0),
      subtitle_disable(0),
      start_time(AV_NOPTS_VALUE),
      duration(AV_NOPTS_VALUE),
      default_width(640),
      default_height(480),
      screen_width(0),
      screen_height(0),
      show_mode(SHOW_MODE_NONE),
      window_title(NULL),
      sws_dict(NULL),
      swr_opts(NULL),
      format_opts(NULL),
      codec_opts(NULL) {}
