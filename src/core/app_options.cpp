#include "core/app_options.h"

#include <stddef.h>  // for NULL

namespace fasto {
namespace fastotv {
namespace core {

AppOptions::AppOptions()
    : autorotate(false),
      seek_by_bytes(-1),
      framedrop(false),
      genpts(false),
      av_sync_type(AV_SYNC_AUDIO_MASTER),
      show_status(true),
      infinite_buffer(-1),
      wanted_stream_spec(),
      lowres(0),
      fast(false),
      audio_codec_name(),
      video_codec_name()
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

ComplexOptions::ComplexOptions(AVDictionary* sws_d,
                               AVDictionary* swr_o,
                               AVDictionary* format_o,
                               AVDictionary* codec_o)
    : sws_dict(NULL), swr_opts(NULL), format_opts(NULL), codec_opts(NULL) {
  if (sws_dict) {
    av_dict_copy(&sws_dict, sws_d, 0);
  }
  if (swr_opts) {
    av_dict_copy(&swr_opts, swr_o, 0);
  }
  if (format_opts) {
    av_dict_copy(&format_opts, format_o, 0);
  }
  if (codec_opts) {
    av_dict_copy(&codec_opts, codec_o, 0);
  }
}

ComplexOptions::~ComplexOptions() {
  av_dict_free(&swr_opts);
  av_dict_free(&sws_dict);
  av_dict_free(&format_opts);
  av_dict_free(&codec_opts);
}

ComplexOptions::ComplexOptions(const ComplexOptions& other)
    : sws_dict(NULL), swr_opts(NULL), format_opts(NULL), codec_opts(NULL) {
  if (other.sws_dict) {
    av_dict_copy(&sws_dict, other.sws_dict, 0);
  }
  if (other.swr_opts) {
    av_dict_copy(&swr_opts, other.swr_opts, 0);
  }
  if (other.format_opts) {
    av_dict_copy(&format_opts, other.format_opts, 0);
  }
  if (other.codec_opts) {
    av_dict_copy(&codec_opts, other.codec_opts, 0);
  }
}

ComplexOptions& ComplexOptions::operator=(const ComplexOptions& rhs) {
  av_dict_free(&swr_opts);
  av_dict_free(&sws_dict);
  av_dict_free(&format_opts);
  av_dict_free(&codec_opts);

  if (rhs.sws_dict) {
    av_dict_copy(&sws_dict, rhs.sws_dict, 0);
  }
  if (rhs.swr_opts) {
    av_dict_copy(&swr_opts, rhs.swr_opts, 0);
  }
  if (rhs.format_opts) {
    av_dict_copy(&format_opts, rhs.format_opts, 0);
  }
  if (rhs.codec_opts) {
    av_dict_copy(&codec_opts, rhs.codec_opts, 0);
  }
  return *this;
}

}  // namespace core
}
}
