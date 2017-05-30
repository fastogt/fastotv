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

#include "client/core/app_options.h"

#include <stddef.h>  // for NULL

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

AppOptions::AppOptions()
    : autorotate(true),
      framedrop(FRAME_DROP_AUTO),
      genpts(false),
      av_sync_type(AV_SYNC_AUDIO_MASTER),
      show_status(true),
      infinite_buffer(-1),
      wanted_stream_spec(),
      lowres(0),
      fast(false),
      audio_codec_name(),
      video_codec_name(),
      hwaccel_id(HWACCEL_NONE),
      hwaccel_device(),
      hwaccel_output_format(),
      auto_exit(true),
      disable_video(false),
      disable_audio(false)
#if CONFIG_AVFILTER
      ,
      vfilters(),
      afilters()
#endif
{
}

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
}
