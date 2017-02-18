#include "core/app_options.h"

#include <stddef.h>  // for NULL

namespace core {

AppOptions::AppOptions()
    : autorotate(false),
      exit_on_keydown(false),
      exit_on_mousedown(false),
      audio_disable(false),
      video_disable(false),
      start_time(AV_NOPTS_VALUE),
      duration(AV_NOPTS_VALUE),
      default_width(width),
      default_height(height),
      screen_width(0),
      screen_height(0),
      show_mode(SHOW_MODE_NONE),
      window_title(),
      framedrop(-1),
      genpts(false),
      av_sync_type(AV_SYNC_AUDIO_MASTER),
      startup_volume(volume),
      seek_by_bytes(-1),
      display_disable(false),
      is_full_screen(false),
      input_filename(),
      loop(loop_count),
      autoexit(false),
      show_status(true),
      infinite_buffer(-1),
      wanted_stream_spec(),
      lowres(0),
      fast(false),
      audio_codec_name(),
      video_codec_name(),
      decoder_reorder_pts(-1)
#if CONFIG_AVFILTER
      ,
      vfilters_list(),
      afilters()
#endif
{
}

#if CONFIG_AVFILTER
void AppOptions::InitAvFilters(const std::string& arg) {
  vfilters_list.push_back(arg);
}
#endif

ComplexOptions::ComplexOptions()
    : sws_dict(NULL), swr_opts(NULL), format_opts(NULL), codec_opts(NULL) {}

}  // namespace core
