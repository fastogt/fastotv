#include "core/app_options.h"

#include <stddef.h>

#if CONFIG_AVFILTER
#include "core/cmd_utils.h"
#endif

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
      framedrop(0),
      genpts(0),
      av_sync_type(AV_SYNC_AUDIO_MASTER),
      startup_volume(100),
      seek_by_bytes(-1),
      display_disable(0),
      is_full_screen(0),
      audio_callback_time(0),
      input_filename(NULL),
      loop(1),
      autoexit(0),
      show_status(1),
      infinite_buffer(-1),
      wanted_stream_spec{0},
      rdftspeed(0.02),
      lowres(0),
      fast(0),
      audio_codec_name(NULL),
      subtitle_codec_name(NULL),
      video_codec_name(NULL),
      decoder_reorder_pts(-1),
#if CONFIG_AVFILTER
      vfilters_list(NULL),
      nb_vfilters(0),
      afilters(NULL),
#endif
      sws_dict(NULL),
      swr_opts(NULL),
      format_opts(NULL),
      codec_opts(NULL) {
}

AppOptions::~AppOptions() {
#if CONFIG_AVFILTER
  av_freep(&vfilters_list);
#endif
}

#if CONFIG_AVFILTER
void AppOptions::initAvFilters(const char* arg) {
  // GROW_ARRAY(vfilters_list, nb_vfilters);
  vfilters_list = static_cast<const char**>(
      grow_array(vfilters_list, sizeof(*vfilters_list), &nb_vfilters, nb_vfilters + 1));
  vfilters_list[nb_vfilters - 1] = arg;
}
#endif
