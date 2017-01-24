#include "core/app_options.h"

#include <stddef.h>

#include "core/utils.h"

namespace core {

AppOptions::AppOptions()
    : autorotate(0),
      exit_on_keydown(0),
      exit_on_mousedown(0),
      audio_disable(0),
      video_disable(0),
      start_time(AV_NOPTS_VALUE),
      duration(AV_NOPTS_VALUE),
      default_width(640),
      default_height(480),
      screen_width(0),
      screen_height(0),
      show_mode(SHOW_MODE_NONE),
      window_title(),
      framedrop(-1),
      genpts(0),
      av_sync_type(AV_SYNC_AUDIO_MASTER),
      startup_volume(100),
      seek_by_bytes(-1),
      display_disable(0),
      is_full_screen(0),
      input_filename(),
      loop(1),
      autoexit(0),
      show_status(1),
      infinite_buffer(-1),
      wanted_stream_spec(),
      lowres(0),
      fast(0),
      audio_codec_name(),
      video_codec_name(),
      decoder_reorder_pts(-1)
#if CONFIG_AVFILTER
      ,
      vfilters_list(),
      afilters(NULL)
#endif
{
}

#if CONFIG_AVFILTER
void AppOptions::initAvFilters(const std::string& arg) {
  vfilters_list.push_back(arg);
}
#endif

ComplexOptions::ComplexOptions()
    : sws_dict(NULL), swr_opts(NULL), format_opts(NULL), codec_opts(NULL) {}

}  // namespace core
