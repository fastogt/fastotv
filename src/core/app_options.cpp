#include "core/app_options.h"

#include <stddef.h>  // for NULL

namespace core {

AppOptions::AppOptions()
    : autorotate(false),
      audio_disable(false),
      video_disable(false),
      seek_by_bytes(-1),
      start_time(AV_NOPTS_VALUE),
      duration(AV_NOPTS_VALUE),
      framedrop(-1),
      genpts(false),
      av_sync_type(AV_SYNC_AUDIO_MASTER),
      startup_volume(volume),
      display_disable(false),
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
