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

#include <inttypes.h>  // for PRIx64
#include <stdio.h>     // for snprintf
#include <string>      // for string

#include "ffmpeg_config.h"  // for CONFIG_AVDEVICE

extern "C" {
#include <libavutil/avutil.h>          // for av_get_media_type_string
#include <libavutil/channel_layout.h>  // for av_get_channel_layout_string
#include <libavutil/dict.h>            // for AVDictionary
#include <libavutil/pixdesc.h>         // for av_get_pix_fmt_name
#include <libavutil/samplefmt.h>       // for av_get_sample_fmt_name
}

#include <common/convert2string.h>  // for ConvertFromString
#include <common/logger.h>          // for COMPACT_LOG_WARNING, WARNING_LOG
#include <common/macros.h>          // for DISALLOW_COPY_AND_ASSIGN

void show_license();
void show_version();
void show_help_tv_player(const std::string& topic);
void show_help_player(const std::string& topic);
void show_buildconf();
void show_formats();
void show_devices();
void show_codecs();
void show_hwaccels();
void show_decoders();
void show_encoders();
void show_bsfs();
void show_protocols();
void show_filters();
void show_pix_fmts();
void show_layouts();
void show_sample_fmts();
void show_colors();

#if CONFIG_AVDEVICE
void show_sinks(const std::string& device);
void show_sources(const std::string& device);
#endif

/**
 * Initialize dynamic library loading
 */
void init_dynload(void);

bool parse_bool(const std::string& bool_str, bool* result);

template <typename T>
bool parse_number(const std::string& number_str, T min, T max, T* result) {
  if (number_str.empty() || !result) {
    WARNING_LOG() << "Can't parse value(number) invalid arguments!";
    return false;
  }

  T lresult;
  if (!common::ConvertFromString(number_str, &lresult)) {
    WARNING_LOG() << "Can't parse value(number) value: " << number_str;
    return false;
  }

  if (lresult < min || lresult > max) {
    WARNING_LOG() << "The value for " << number_str << " was " << lresult << " which is not within " << min << " - "
                  << max;
    return false;
  }

  *result = lresult;
  return true;
}

#define media_type_string av_get_media_type_string

#define GET_PIX_FMT_NAME(pix_fmt) const char* name = av_get_pix_fmt_name(pix_fmt);

#define GET_SAMPLE_FMT_NAME(sample_fmt) const char* name = av_get_sample_fmt_name(sample_fmt)

#define GET_SAMPLE_RATE_NAME(rate) \
  char name[16];                   \
  snprintf(name, sizeof(name), "%d", rate);

#define GET_CH_LAYOUT_NAME(ch_layout) \
  char name[16];                      \
  snprintf(name, sizeof(name), "0x%" PRIx64, ch_layout);

#define GET_CH_LAYOUT_DESC(ch_layout) \
  char name[128];                     \
  av_get_channel_layout_string(name, sizeof(name), 0, ch_layout);
