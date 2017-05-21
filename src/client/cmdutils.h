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

#include "ffmpeg_config.h"  // for CONFIG_AVDEVICE, etc

extern "C" {
#include <libavutil/dict.h>       // for AVDictionary
}

#include <common/convert2string.h>  // for ConvertFromString
#include <common/macros.h>          // for DISALLOW_COPY_AND_ASSIGN

void show_license();
void show_version();
void show_usage();
void show_help(const std::string& topic);
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

struct DictionaryOptions {
  DictionaryOptions();
  ~DictionaryOptions();

  AVDictionary* sws_dict;
  AVDictionary* swr_opts;
  AVDictionary* format_opts;
  AVDictionary* codec_opts;

 private:
  DISALLOW_COPY_AND_ASSIGN(DictionaryOptions);
};

/**
 * Initialize dynamic library loading
 */
void init_dynload(void);

/**
 * Fallback for options that are not explicitly handled, these will be
 * parsed through AVOptions.
 */
int opt_default(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Set the libav* libraries log level.
 */
int opt_loglevel(const char* opt, const char* arg, DictionaryOptions* dopt);

int opt_codec_debug(const char* opt, const char* arg, DictionaryOptions* dopt);

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
    WARNING_LOG() << "The value for " << number_str << " was " << lresult << " which is not within "
                  << min << " - " << max;
    return false;
  }

  *result = lresult;
  return true;
}

/**
 * Print the version of the program to stdout. The version message
 * depends on the current versions of the repository and of the libav*
 * libraries.
 * This option processing function does not utilize the arguments.
 */
int show_version(const char* opt, const char* arg, DictionaryOptions* dopt);

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
