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

#include "client/cmdutils.h"

#ifdef _WIN32
#include <windef.h>

#include <winbase.h>
#endif

#include <errno.h>   // for EINVAL, ENOMEM, ENOSYS
#include <stddef.h>  // for size_t
#include <stdint.h>  // for int64_t, uint64_t, uint8_t, etc
#include <string.h>  // for strcmp, strchr, strlen, etc

#include <iostream>

/* Include only the enabled headers since some compilers (namely, Sun
   Studio) will not omit unused inline functions and create undefined
   references to libraries that are not being built. */

extern "C" {
#include <libavcodec/avcodec.h>        // for AVCodec, AVCodecDescriptor
#include <libavcodec/version.h>        // for LIBAVCODEC_VERSION_MAJOR, etc
#include <libavdevice/avdevice.h>      // for AVDeviceInfoList, etc
#include <libavdevice/version.h>       // for LIBAVDEVICE_VERSION_MAJOR, etc
#include <libavfilter/avfilter.h>      // for AVFilter, etc
#include <libavfilter/version.h>       // for LIBAVFILTER_VERSION_MAJOR, etc
#include <libavformat/avformat.h>      // for AVOutputFormat, etc
#include <libavformat/avio.h>          // for avio_enum_protocols
#include <libavformat/version.h>       // for LIBAVFORMAT_VERSION_MAJOR, etc
#include <libavutil/avstring.h>        // for av_match_name, av_strlcat, etc
#include <libavutil/common.h>          // for AVERROR, etc
#include <libavutil/dict.h>            // for av_dict_free, AVDictionary, etc
#include <libavutil/error.h>           // for AVERROR, etc
#include <libavutil/ffversion.h>       // for FFMPEG_VERSION
#include <libavutil/log.h>             // for AVClass, AV_IS_INPUT_DEVICE
#include <libavutil/mem.h>             // for av_free, av_freep, etc
#include <libavutil/parseutils.h>      // for av_get_known_color_name, etc
#include <libavutil/pixdesc.h>         // for AVPixFmtDescriptor, etc
#include <libavutil/pixfmt.h>          // for AVPixelFormat, etc
#include <libavutil/rational.h>        // for AVRational
#include <libavutil/version.h>         // for AV_VERSION_MAJOR, etc
#include <libswresample/swresample.h>  // for swr_alloc, swr_free, etc
#include <libswresample/version.h>     // for LIBSWRESAMPLE_VERSION_MAJOR, etc
#include <libswscale/swscale.h>        // for sws_alloc_context, etc
#include <libswscale/version.h>        // for LIBSWSCALE_VERSION_MAJOR, etc
}

#include <common/log_levels.h>  // for LEVEL_LOG::L_INFO, etc
#include <common/sprintf.h>     // for MemSPrintf
#include <common/utils.h>

#include "client/core/ffmpeg_internal.h"

#if CONFIG_AVDEVICE
#define HELP_AVDEVICE                                          \
  "    --sources [device]  list sources of the input device\n" \
  "    --sinks [device]  list sinks of the output device\n"
#else
#define HELP_AVDEVICE
#endif

#define HELP_TEXT                                            \
  "Usage: " PROJECT_NAME                                     \
  " [options]\n"                                             \
  "    --version  show version\n"                            \
  "    --help [topic]  show help\n"                          \
  "    --license  show license\n"                            \
  "    --buildconf  show build configuration\n"              \
  "    --formats  show available formats\n"                  \
  "    --devices  show available devices\n"                  \
  "    --codecs  show available codecs\n"                    \
  "    --hwaccels  show available hwaccels\n"                \
  "    --decoders  show available decoders\n"                \
  "    --encoders  show available encoders\n"                \
  "    --bsfs  show available bit stream filters\n"          \
  "    --protocols  show available protocols\n"              \
  "    --filters  show available filters\n"                  \
  "    --pix_fmts  show available pixel formats\n"           \
  "    --layouts  show standard channel layouts\n"           \
  "    --sample_fmts  show available audio sample formats\n" \
  "    --colors  show available color names\n" HELP_AVDEVICE

namespace {
bool warned_cfg = false;

bool compare_codec_desc(const AVCodecDescriptor* da, const AVCodecDescriptor* db) {
  if ((da)->type == (db)->type) {
    return strcmp((da)->name, (db)->name) < 0;
  }
  return (da)->type < (db)->type;
}

bool is_device(const AVClass* avclass) {
  if (!avclass) {
    return false;
  }
  return AV_IS_INPUT_DEVICE(avclass->category) || AV_IS_OUTPUT_DEVICE(avclass->category);
}

char get_media_type_char(enum AVMediaType type) {
  if (type == AVMEDIA_TYPE_VIDEO) {
    return 'V';
  } else if (type == AVMEDIA_TYPE_AUDIO) {
    return 'A';
  } else if (type == AVMEDIA_TYPE_DATA) {
    return 'D';
  } else if (type == AVMEDIA_TYPE_SUBTITLE) {
    return 'S';
  } else if (type == AVMEDIA_TYPE_ATTACHMENT) {
    return 'T';
  }

  return '?';
}

bool get_codecs_sorted(std::vector<const AVCodecDescriptor*>* rcodecs) {
  if (!rcodecs) {
    return false;
  }

  std::vector<const AVCodecDescriptor*> lcodecs;
  const AVCodecDescriptor* desc = NULL;
  while ((desc = avcodec_descriptor_next(desc))) {
    lcodecs.push_back(desc);
  }

  std::sort(lcodecs.begin(), lcodecs.end(), compare_codec_desc);
  *rcodecs = lcodecs;
  return true;
}

#define INDENT 1
#define SHOW_VERSION 2
#define SHOW_CONFIG 4
#define SHOW_COPYRIGHT 8

#define PRINT_LIB_INFO(libname, LIBNAME, flags, level)                                                          \
  if (CONFIG_##LIBNAME) {                                                                                       \
    const char* indent = (flags & INDENT) ? "  " : "";                                                          \
    if (flags & SHOW_VERSION) {                                                                                 \
      unsigned int version = libname##_version();                                                               \
      RUNTIME_LOG(level) << common::MemSPrintf("%slib%-11s %2d.%3d.%3d / %2d.%3d.%3d", indent, #libname,        \
                                               LIB##LIBNAME##_VERSION_MAJOR, LIB##LIBNAME##_VERSION_MINOR,      \
                                               LIB##LIBNAME##_VERSION_MICRO, AV_VERSION_MAJOR(version),         \
                                               AV_VERSION_MINOR(version), AV_VERSION_MICRO(version));           \
    }                                                                                                           \
    if (flags & SHOW_CONFIG) {                                                                                  \
      const char* cfg = libname##_configuration();                                                              \
      if (strcmp(FFMPEG_CONFIGURATION, cfg)) {                                                                  \
        if (!warned_cfg) {                                                                                      \
          RUNTIME_LOG(level) << common::MemSPrintf("%sWARNING: library configuration mismatch", indent);        \
          warned_cfg = true;                                                                                    \
        }                                                                                                       \
        RUNTIME_LOG(level) << indent << common::MemSPrintf("%s%-11s configuration: %s", indent, #libname, cfg); \
      }                                                                                                         \
    }                                                                                                           \
  }

void print_all_libs_info(int flags, common::logging::LEVEL_LOG level) {
  PRINT_LIB_INFO(avutil, AVUTIL, flags, level);
  PRINT_LIB_INFO(avcodec, AVCODEC, flags, level);
  PRINT_LIB_INFO(avformat, AVFORMAT, flags, level);
  PRINT_LIB_INFO(avdevice, AVDEVICE, flags, level);
  PRINT_LIB_INFO(avfilter, AVFILTER, flags, level);
  //    PRINT_LIB_INFO(avresample, AVRESAMPLE, flags, level);
  PRINT_LIB_INFO(swscale, SWSCALE, flags, level);
  PRINT_LIB_INFO(swresample, SWRESAMPLE, flags, level);
  //    PRINT_LIB_INFO(postproc,   POSTPROC,   flags, level);
}

void print_program_info(int flags, common::logging::LEVEL_LOG level) {
  const char* indent = (flags & INDENT) ? "  " : "";
  RUNTIME_LOG(level) << PROJECT_NAME_TITLE " version: " PROJECT_VERSION;
  if (flags & SHOW_COPYRIGHT) {
    RUNTIME_LOG(level) << " " PROJECT_COPYRIGHT;
  }
  RUNTIME_LOG(level) << indent << "built with " << CC_IDENT;
  RUNTIME_LOG(level) << indent << "FFMPEG version " FFMPEG_VERSION ", configuration: " FFMPEG_CONFIGURATION;
}

void print_buildconf(int flags) {
  const char* indent = (flags & INDENT) ? "  " : "";
  char str[] = {FFMPEG_CONFIGURATION};
  char *conflist, *remove_tilde, *splitconf;

  // Change all the ' --' strings to '~--' so that
  // they can be identified as tokens.
  while ((conflist = strstr(str, " --")) != NULL) {
    strncpy(conflist, "~--", 3);
  }

  // Compensate for the weirdness this would cause
  // when passing 'pkg-config --static'.
  while ((remove_tilde = strstr(str, "pkg-config~")) != NULL) {
    strncpy(remove_tilde, "pkg-config ", 11);
  }

  splitconf = strtok(str, "~");
  std::cout << "\n" << indent << " FFMPEG configuration:";
  while (splitconf != NULL) {
    std::cout << indent << indent << splitconf;
    splitconf = strtok(NULL, "~");
  }
}

const AVCodec* next_codec_for_id(enum AVCodecID id, const AVCodec* prev, bool encoder) {
  while ((prev = av_codec_next(prev))) {
    if (prev->id == id && (encoder ? av_codec_is_encoder(prev) : av_codec_is_decoder(prev))) {
      return prev;
    }
  }
  return NULL;
}

void print_codecs_for_id(enum AVCodecID id, bool encoder) {
  const AVCodec* codec = NULL;

  printf(" (%s: ", encoder ? "encoders" : "decoders");

  while ((codec = next_codec_for_id(id, codec, encoder))) {
    printf("%s ", codec->name);
  }

  printf(")");
}

void print_codecs(bool encoder) {
  std::cout << (encoder ? "Encoders" : "Decoders") << ":\n"
                                                      " V..... = Video\n"
                                                      " A..... = Audio\n"
                                                      " S..... = Subtitle\n"
                                                      " .F.... = Frame-level multithreading\n"
                                                      " ..S... = Slice-level multithreading\n"
                                                      " ...X.. = Codec is experimental\n"
                                                      " ....B. = Supports draw_horiz_band\n"
                                                      " .....D = Supports direct rendering method 1\n"
                                                      " ------"
            << std::endl;

  std::vector<const AVCodecDescriptor*> codecs;
  bool is_ok = get_codecs_sorted(&codecs);
  if (!is_ok) {
    return;
  }
  for (const AVCodecDescriptor* desc : codecs) {
    const AVCodec* codec = NULL;

    while ((codec = next_codec_for_id(desc->id, codec, encoder))) {
      printf(" %c", get_media_type_char(desc->type));
      printf((codec->capabilities & AV_CODEC_CAP_FRAME_THREADS) ? "F" : ".");
      printf((codec->capabilities & AV_CODEC_CAP_SLICE_THREADS) ? "S" : ".");
      printf((codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL) ? "X" : ".");
      printf((codec->capabilities & AV_CODEC_CAP_DRAW_HORIZ_BAND) ? "B" : ".");
      printf((codec->capabilities & AV_CODEC_CAP_DR1) ? "D" : ".");

      printf(" %-20s %s", codec->name, codec->long_name ? codec->long_name : "");
      if (strcmp(codec->name, desc->name)) {
        printf(" (codec %s)", desc->name);
      }

      printf("\n");
    }
  }
}

void show_formats_devices(bool device_only) {
  AVInputFormat* ifmt = NULL;
  AVOutputFormat* ofmt = NULL;
  std::cout << (device_only ? "Devices:" : "File formats:") << "\n"
                                                               " D. = Demuxing supported\n"
                                                               " .E = Muxing supported\n"
                                                               " --"
            << std::endl;

  const char* last_name = "000";
  for (;;) {
    bool decode = false;
    bool encode = false;
    const char* name = NULL;
    const char* long_name = NULL;

    while ((ofmt = av_oformat_next(ofmt))) {
      bool is_dev = is_device(ofmt->priv_class);
      if (!is_dev && device_only)
        continue;
      if ((!name || strcmp(ofmt->name, name) < 0) && strcmp(ofmt->name, last_name) > 0) {
        name = ofmt->name;
        long_name = ofmt->long_name;
        encode = true;
      }
    }
    while ((ifmt = av_iformat_next(ifmt))) {
      bool is_dev = is_device(ifmt->priv_class);
      if (!is_dev && device_only) {
        continue;
      }
      if ((!name || strcmp(ifmt->name, name) < 0) && strcmp(ifmt->name, last_name) > 0) {
        name = ifmt->name;
        long_name = ifmt->long_name;
        encode = false;
      }
      if (name && strcmp(ifmt->name, name) == 0) {
        decode = true;
      }
    }
    if (!name) {
      break;
    }
    last_name = name;

    printf(" %s%s %-15s %s\n", decode ? "D" : " ", encode ? "E" : " ", name, long_name ? long_name : " ");
  }
}

#define PRINT_CODEC_SUPPORTED(codec, field, type, list_name, term, get_name) \
  if (codec->field) {                                                        \
    const type* p = codec->field;                                            \
                                                                             \
    printf("    Supported " list_name ":");                                  \
    while (*p != term) {                                                     \
      get_name(*p);                                                          \
      printf(" %s", name);                                                   \
      p++;                                                                   \
    }                                                                        \
    printf("\n");                                                            \
  }

void print_codec(const AVCodec* c) {
  int encoder = av_codec_is_encoder(c);

  printf("%s %s [%s]:\n", encoder ? "Encoder" : "Decoder", c->name, c->long_name ? c->long_name : "");

  printf("    General capabilities: ");
  if (c->capabilities & AV_CODEC_CAP_DRAW_HORIZ_BAND) {
    printf("horizband ");
  }
  if (c->capabilities & AV_CODEC_CAP_DR1) {
    printf("dr1 ");
  }
  if (c->capabilities & AV_CODEC_CAP_TRUNCATED) {
    printf("trunc ");
  }
  if (c->capabilities & AV_CODEC_CAP_DELAY) {
    printf("delay ");
  }
  if (c->capabilities & AV_CODEC_CAP_SMALL_LAST_FRAME) {
    printf("small ");
  }
  if (c->capabilities & AV_CODEC_CAP_SUBFRAMES) {
    printf("subframes ");
  }
  if (c->capabilities & AV_CODEC_CAP_EXPERIMENTAL) {
    printf("exp ");
  }
  if (c->capabilities & AV_CODEC_CAP_CHANNEL_CONF) {
    printf("chconf ");
  }
  if (c->capabilities & AV_CODEC_CAP_PARAM_CHANGE) {
    printf("paramchange ");
  }
  if (c->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) {
    printf("variable ");
  }
  if (c->capabilities & (AV_CODEC_CAP_FRAME_THREADS | AV_CODEC_CAP_SLICE_THREADS | AV_CODEC_CAP_AUTO_THREADS)) {
    printf("threads ");
  }
  if (!c->capabilities) {
    printf("none");
  }
  printf("\n");

  if (c->type == AVMEDIA_TYPE_VIDEO || c->type == AVMEDIA_TYPE_AUDIO) {
    printf("    Threading capabilities: ");
    switch (c->capabilities & (AV_CODEC_CAP_FRAME_THREADS | AV_CODEC_CAP_SLICE_THREADS | AV_CODEC_CAP_AUTO_THREADS)) {
      case AV_CODEC_CAP_FRAME_THREADS | AV_CODEC_CAP_SLICE_THREADS:
        printf("frame and slice");
        break;
      case AV_CODEC_CAP_FRAME_THREADS:
        printf("frame");
        break;
      case AV_CODEC_CAP_SLICE_THREADS:
        printf("slice");
        break;
      case AV_CODEC_CAP_AUTO_THREADS:
        printf("auto");
        break;
      default:
        printf("none");
        break;
    }
    printf("\n");
  }

  if (c->supported_framerates) {
    const AVRational* fps = c->supported_framerates;

    printf("    Supported framerates:");
    while (fps->num) {
      printf(" %d/%d", fps->num, fps->den);
      fps++;
    }
    printf("\n");
  }
  PRINT_CODEC_SUPPORTED(c, pix_fmts, enum AVPixelFormat, "pixel formats", AV_PIX_FMT_NONE, GET_PIX_FMT_NAME);
  PRINT_CODEC_SUPPORTED(c, supported_samplerates, int, "sample rates", 0, GET_SAMPLE_RATE_NAME);
  PRINT_CODEC_SUPPORTED(c, sample_fmts, enum AVSampleFormat, "sample formats", AV_SAMPLE_FMT_NONE, GET_SAMPLE_FMT_NAME);
  PRINT_CODEC_SUPPORTED(c, channel_layouts, uint64_t, "channel layouts", 0, GET_CH_LAYOUT_DESC);
}

void show_help_codec(const std::string& name, bool encoder) {
  if (name.empty()) {
    std::cout << "No codec name specified." << std::endl;
    return;
  }

  const char* name_ptr = name.c_str();
  const AVCodec* codec = encoder ? avcodec_find_encoder_by_name(name_ptr) : avcodec_find_decoder_by_name(name_ptr);

  if (codec) {
    print_codec(codec);
    return;
  }

  const AVCodecDescriptor* desc = avcodec_descriptor_get_by_name(name_ptr);
  if (desc) {
    bool printed = false;

    while ((codec = next_codec_for_id(desc->id, codec, encoder))) {
      printed = true;
      print_codec(codec);
    }

    if (!printed) {
      std::cout << "Codec '" << name << "' is known to FFmpeg, "
                << "but no " << (encoder ? "encoders" : "decoders")
                << " for it are available. FFmpeg might need to be recompiled with additional "
                   "external libraries."
                << std::endl;
    }
  } else {
    std::cout << "Codec '" << name << "' is not recognized by FFmpeg." << std::endl;
  }
}

void show_help_demuxer(const std::string& name) {
  const AVInputFormat* fmt = av_find_input_format(name.c_str());

  if (!fmt) {
    std::cout << "Unknown format '" << name << "'." << std::endl;
    return;
  }

  std::cout << "Demuxer " << fmt->name << " [" << fmt->long_name << "]:" << std::endl;

  if (fmt->extensions) {
    std::cout << "    Common extensions: " << fmt->extensions << "." << std::endl;
  }
}

void show_help_muxer(const std::string& name) {
  const AVCodecDescriptor* desc;
  const AVOutputFormat* fmt = av_guess_format(name.c_str(), NULL, NULL);

  if (!fmt) {
    std::cout << "Unknown format '" << name << "'." << std::endl;
    return;
  }

  std::cout << "Muxer " << fmt->name << " [" << fmt->long_name << "]:" << std::endl;

  if (fmt->extensions) {
    std::cout << "    Common extensions: " << fmt->extensions << "." << std::endl;
  }
  if (fmt->mime_type) {
    std::cout << "    Mime type: " << fmt->mime_type << "." << std::endl;
  }
  if (fmt->video_codec != AV_CODEC_ID_NONE && (desc = avcodec_descriptor_get(fmt->video_codec))) {
    std::cout << "    Default video codec: " << desc->name << "." << std::endl;
  }
  if (fmt->audio_codec != AV_CODEC_ID_NONE && (desc = avcodec_descriptor_get(fmt->audio_codec))) {
    std::cout << "    Default audio codec: " << desc->name << "." << std::endl;
  }
  if (fmt->subtitle_codec != AV_CODEC_ID_NONE && (desc = avcodec_descriptor_get(fmt->subtitle_codec))) {
    std::cout << "    Default subtitle codec: " << desc->name << "." << std::endl;
  }
}

void show_help_filter(const std::string& name) {
#if CONFIG_AVFILTER
  if (name.empty()) {
    std::cout << "No filter name specified." << std::endl;
    return;
  }

  const AVFilter* f = avfilter_get_by_name(name.c_str());
  if (!f) {
    std::cout << "Unknown filter '" << name << "'." << std::endl;
    return;
  }

  std::cout << "Filter " << f->name << std::endl;
  if (f->description) {
    std::cout << "  " << f->description << std::endl;
  }

  if (f->flags & AVFILTER_FLAG_SLICE_THREADS) {
    std::cout << "    slice threading supported" << std::endl;
  }

  printf("    Inputs:\n");
  int count = avfilter_pad_count(f->inputs);
  for (int i = 0; i < count; i++) {
    std::cout << "       #" << i << ": " << avfilter_pad_get_name(f->inputs, i) << " ("
              << media_type_string(avfilter_pad_get_type(f->inputs, i)) << ")" << std::endl;
  }
  if (f->flags & AVFILTER_FLAG_DYNAMIC_INPUTS) {
    std::cout << "        dynamic (depending on the options)" << std::endl;
  } else if (!count) {
    std::cout << "        none (source filter)" << std::endl;
  }

  std::cout << "    Outputs:" << std::endl;
  count = avfilter_pad_count(f->outputs);
  for (int i = 0; i < count; i++) {
    printf("       #%d: %s (%s)\n", i, avfilter_pad_get_name(f->outputs, i),
           media_type_string(avfilter_pad_get_type(f->outputs, i)));
  }
  if (f->flags & AVFILTER_FLAG_DYNAMIC_OUTPUTS) {
    std::cout << "        dynamic (depending on the options)" << std::endl;
  } else if (!count) {
    std::cout << "        none (sink filter)" << std::endl;
  }

  if (f->flags & AVFILTER_FLAG_SUPPORT_TIMELINE) {
    std::cout << "This filter has support for timeline through the 'enable' option." << std::endl;
  }
#else
  std::cout << "Build without libavfilter; can not to satisfy request" << std::endl;
#endif
}

void show_help_default() {
  show_usage();
  std::cout << "\nWhile playing:\n"
               "q, ESC              quit\n"
               "f                   toggle full screen\n"
               "p, SPC              pause\n"
               "m                   toggle mute\n"
               "9, 0                decrease and increase volume respectively\n"
               "/, *                decrease and increase volume respectively\n"
               "[, ]                prev/next channel\n"
               "F3                  channel statistic\n"
               "a                   cycle audio channel in the current program\n"
               "v                   cycle video channel\n"
               "c                   cycle program\n"
               "w                   cycle video filters or show modes\n"
               "s                   activate frame-step mode\n"
               "left double-click   toggle full screen"
            << std::endl;
}
}  // namespace

void show_license() {
  std::cout << PROJECT_NAME_TITLE
      " is free software; you can redistribute it and/or modify\n"
      "it under the terms of the GNU Lesser General Public License as published by\n"
      "the Free Software Foundation; either version 3 of the License, or\n"
      "(at your option) any later version.\n"
      "\n" PROJECT_NAME_TITLE
      " is distributed in the hope that it will be useful,\n"
      "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
      "GNU Lesser General Public License for more details.\n"
      "\n"
      "You should have received a copy of the GNU Lesser General Public License\n"
      "along with " PROJECT_NAME_TITLE ".  If not, see <http://www.gnu.org/licenses/>."
            << std::endl;
}

void show_version() {
  print_program_info(SHOW_COPYRIGHT, common::logging::L_INFO);
  print_all_libs_info(SHOW_VERSION, common::logging::L_INFO);
}

void show_buildconf() {
  print_buildconf(INDENT | 0);
}

void show_formats() {
  show_formats_devices(false);
}

void show_devices() {
  show_formats_devices(true);
}

void show_codecs() {
  std::cout << "Codecs:\n"
               " D..... = Decoding supported\n"
               " .E.... = Encoding supported\n"
               " ..V... = Video codec\n"
               " ..A... = Audio codec\n"
               " ..S... = Subtitle codec\n"
               " ...I.. = Intra frame-only codec\n"
               " ....L. = Lossy compression\n"
               " .....S = Lossless compression\n"
               " -------"
            << std::endl;

  std::vector<const AVCodecDescriptor*> codecs;
  bool is_ok = get_codecs_sorted(&codecs);
  if (!is_ok) {
    return;
  }

  for (const AVCodecDescriptor* desc : codecs) {
    const AVCodec* codec = NULL;

    if (strstr(desc->name, "_deprecated")) {
      continue;
    }

    printf(" ");
    printf(avcodec_find_decoder(desc->id) ? "D" : ".");
    printf(avcodec_find_encoder(desc->id) ? "E" : ".");

    printf("%c", get_media_type_char(desc->type));
    printf((desc->props & AV_CODEC_PROP_INTRA_ONLY) ? "I" : ".");
    printf((desc->props & AV_CODEC_PROP_LOSSY) ? "L" : ".");
    printf((desc->props & AV_CODEC_PROP_LOSSLESS) ? "S" : ".");

    printf(" %-20s %s", desc->name, desc->long_name ? desc->long_name : "");

    /* print decoders/encoders when there's more than one or their
     * names are different from codec name */
    while ((codec = next_codec_for_id(desc->id, codec, 0))) {
      if (strcmp(codec->name, desc->name)) {
        print_codecs_for_id(desc->id, 0);
        break;
      }
    }
    codec = NULL;
    while ((codec = next_codec_for_id(desc->id, codec, 1))) {
      if (strcmp(codec->name, desc->name)) {
        print_codecs_for_id(desc->id, 1);
        break;
      }
    }

    printf("\n");
  }
}

void show_decoders() {
  print_codecs(false);
}

void show_encoders() {
  print_codecs(true);
}

void show_bsfs() {
  const AVBitStreamFilter* bsf = NULL;
  void* opaque = NULL;

  std::cout << "Bitstream filters:" << std::endl;
  while ((bsf = av_bsf_next(&opaque))) {
    std::cout << bsf->name << std::endl;
  }
}

void show_hwaccels() {
  std::cout << "Hardware acceleration methods:" << std::endl;
  for (size_t i = 0; i < fasto::fastotv::client::core::hwaccel_count(); i++) {
    std::cout << fasto::fastotv::client::core::hwaccels[i].name << std::endl;
  }
}

void show_protocols() {
  void* opaque = NULL;
  const char* name;

  std::cout << "Supported file protocols:\n"
               "Input:"
            << std::endl;
  while ((name = avio_enum_protocols(&opaque, 0))) {
    std::cout << "  " << name << std::endl;
  }
  std::cout << "Output:" << std::endl;
  while ((name = avio_enum_protocols(&opaque, 1))) {
    std::cout << "  " << name << std::endl;
  }
}

void show_filters() {
#if CONFIG_AVFILTER
  const AVFilter* filter = NULL;
  char descr[64];
  const AVFilterPad* pad;

  std::cout << "Filters:\n"
               "  T.. = Timeline support\n"
               "  .S. = Slice threading\n"
               "  ..C = Command support\n"
               "  A = Audio input/output\n"
               "  V = Video input/output\n"
               "  N = Dynamic number and/or type of input/output\n"
               "  | = Source or sink filter"
            << std::endl;
  while ((filter = avfilter_next(filter))) {
    char* descr_cur = descr;
    for (int i = 0; i < 2; i++) {
      if (i) {
        *(descr_cur++) = '-';
        *(descr_cur++) = '>';
      }
      pad = i ? filter->outputs : filter->inputs;
      int j;
      for (j = 0; pad && avfilter_pad_get_name(pad, j); j++) {
        if (descr_cur >= descr + sizeof(descr) - 4) {
          break;
        }
        *(descr_cur++) = get_media_type_char(avfilter_pad_get_type(pad, j));
      }
      if (!j)
        *(descr_cur++) = ((!i && (filter->flags & AVFILTER_FLAG_DYNAMIC_INPUTS)) ||
                          (i && (filter->flags & AVFILTER_FLAG_DYNAMIC_OUTPUTS)))
                             ? 'N'
                             : '|';
    }
    *descr_cur = 0;
    printf(" %c%c%c %-17s %-10s %s\n", (filter->flags & AVFILTER_FLAG_SUPPORT_TIMELINE) ? 'T' : '.',
           (filter->flags & AVFILTER_FLAG_SLICE_THREADS) ? 'S' : '.', filter->process_command ? 'C' : '.', filter->name,
           descr, filter->description);
  }
#else
  std::cout << "No filters available: libavfilter disabled" << std::endl;
#endif
}

void show_pix_fmts() {
  const AVPixFmtDescriptor* pix_desc = NULL;

  std::cout << "Pixel formats:\n"
               "I.... = Supported Input  format for conversion\n"
               ".O... = Supported Output format for conversion\n"
               "..H.. = Hardware accelerated format\n"
               "...P. = Paletted format\n"
               "....B = Bitstream format\n"
               "FLAGS NAME            NB_COMPONENTS BITS_PER_PIXEL\n"
               "-----"
            << std::endl;

#if !CONFIG_SWSCALE
#define sws_isSupportedInput(x) 0
#define sws_isSupportedOutput(x) 0
#endif

  while ((pix_desc = av_pix_fmt_desc_next(pix_desc))) {
    enum AVPixelFormat pix_fmt = av_pix_fmt_desc_get_id(pix_desc);
    printf("%c%c%c%c%c %-16s       %d            %2d\n", sws_isSupportedInput(pix_fmt) ? 'I' : '.',
           sws_isSupportedOutput(pix_fmt) ? 'O' : '.', (pix_desc->flags & AV_PIX_FMT_FLAG_HWACCEL) ? 'H' : '.',
           (pix_desc->flags & AV_PIX_FMT_FLAG_PAL) ? 'P' : '.',
           (pix_desc->flags & AV_PIX_FMT_FLAG_BITSTREAM) ? 'B' : '.', pix_desc->name, pix_desc->nb_components,
           av_get_bits_per_pixel(pix_desc));
  }
}

void show_layouts() {
  const char* name = NULL;

  std::cout << "Individual channels:\n"
               "NAME           DESCRIPTION"
            << std::endl;
  for (int i = 0; i < 63; i++) {
    name = av_get_channel_name(uint64_t(1) << i);
    if (!name) {
      continue;
    }
    const char* descr = av_get_channel_description(static_cast<uint64_t>(1) << i);
    printf("%-14s %s\n", name, descr);
  }
  std::cout << "\nStandard channel layouts:\n"
               "NAME           DECOMPOSITION"
            << std::endl;
  uint64_t layout;
  for (unsigned i = 0; !av_get_standard_channel_layout(i, &layout, &name); i++) {
    if (name) {
      printf("%-14s ", name);
      for (uint64_t j = 1; j; j <<= 1)
        if ((layout & j)) {
          printf("%s%s", (layout & (j - 1)) ? "+" : "", av_get_channel_name(j));
        }
      std::cout << std::endl;
    }
  }
}

void show_sample_fmts() {
  for (int i = -1; i < AV_SAMPLE_FMT_NB; i++) {
    char fmt_str[128] = {0};
    const AVSampleFormat sample_fmt = static_cast<AVSampleFormat>(i);
    const char* str = av_get_sample_fmt_string(fmt_str, sizeof(fmt_str), sample_fmt);
    std::cout << str << std::endl;
  }
}

void show_colors() {
  const char* name;
  const uint8_t* rgb;
  printf("%-32s #RRGGBB\n", "name");

  for (int i = 0; (name = av_get_known_color_name(i, &rgb)); i++) {
    printf("%-32s #%02x%02x%02x\n", name, rgb[0], rgb[1], rgb[2]);
  }
}

void show_usage() {
  std::cout << "Simple media player\nusage: " PROJECT_NAME_TITLE " [options]" << std::endl;
}

void show_help(const std::string& topic) {
  if (topic.empty()) {
    std::cout << HELP_TEXT << std::endl;
    return;
  }

  size_t del = topic.find_first_of('=');
  std::string par;
  std::string stabled_topic = topic;
  if (del != std::string::npos) {
    par = topic.substr(del + 1);
    stabled_topic = topic.substr(0, del);
  }

  if (stabled_topic == "decoder") {
    show_help_codec(par, 0);
  } else if (stabled_topic == "encoder") {
    show_help_codec(par, 1);
  } else if (stabled_topic == "demuxer") {
    show_help_demuxer(par);
  } else if (stabled_topic == "muxer") {
    show_help_muxer(par);
#if CONFIG_AVFILTER
  } else if (stabled_topic == "filter") {
    show_help_filter(par);
#endif
  } else {
    show_help_default();
  }
}

void init_dynload(void) {
#ifdef _WIN32
  /* Calling SetDllDirectory with the empty string (but not NULL) removes the
   * current working directory from the DLL search path as a security pre-caution. */
  SetDllDirectory("");
#endif
}

bool parse_bool(const std::string& bool_str, bool* result) {
  if (bool_str.empty()) {
    WARNING_LOG() << "Can't parse value(bool) invalid arguments!";
    return false;
  }

  std::string bool_str_copy = bool_str;
  std::transform(bool_str_copy.begin(), bool_str_copy.end(), bool_str_copy.begin(), ::tolower);
  if (result) {
    *result = bool_str_copy == "true" || bool_str_copy == "on";
  }
  return true;
}

#if CONFIG_AVDEVICE
namespace {
int print_device_sources(AVInputFormat* fmt, AVDictionary* opts) {
  if (!fmt || !fmt->priv_class || !AV_IS_INPUT_DEVICE(fmt->priv_class->category)) {
    return AVERROR(EINVAL);
  }

  AVDeviceInfoList* device_list = NULL;
  printf("Auto-detected sources for %s:\n", fmt->name);
  if (!fmt->get_device_list) {
    printf("Cannot list sources. Not implemented.\n");
    avdevice_free_list_devices(&device_list);
    return AVERROR(ENOSYS);
  }

  int ret = avdevice_list_input_sources(fmt, NULL, opts, &device_list);
  if (ret < 0) {
    printf("Cannot list sources.\n");
    avdevice_free_list_devices(&device_list);
    return ret;
  }

  for (int i = 0; i < device_list->nb_devices; i++) {
    printf("%s %s [%s]\n", device_list->default_device == i ? "*" : " ", device_list->devices[i]->device_name,
           device_list->devices[i]->device_description);
  }

  avdevice_free_list_devices(&device_list);
  return ret;
}

int print_device_sinks(AVOutputFormat* fmt, AVDictionary* opts) {
  if (!fmt || !fmt->priv_class || !AV_IS_OUTPUT_DEVICE(fmt->priv_class->category)) {
    return AVERROR(EINVAL);
  }

  AVDeviceInfoList* device_list = NULL;
  printf("Auto-detected sinks for %s:\n", fmt->name);
  if (!fmt->get_device_list) {
    printf("Cannot list sinks. Not implemented.\n");
    return AVERROR(ENOSYS);
  }

  int ret = avdevice_list_output_sinks(fmt, NULL, opts, &device_list);
  if (ret < 0) {
    printf("Cannot list sinks.\n");
    avdevice_free_list_devices(&device_list);
    return ret;
  }

  for (int i = 0; i < device_list->nb_devices; i++) {
    printf("%s %s [%s]\n", device_list->default_device == i ? "*" : " ", device_list->devices[i]->device_name,
           device_list->devices[i]->device_description);
  }

  avdevice_free_list_devices(&device_list);
  return ret;
}

int show_sinks_sources_parse_arg(const char* arg, char** dev, AVDictionary** opts) {
  if (!arg) {
    printf(
        "\nDevice name is not provided.\n"
        "You can pass devicename[,opt1=val1[,opt2=val2...]] as an argument.\n\n");
    return 0;
  }

  char* opts_str = NULL;
  CHECK(dev && opts);
  *dev = av_strdup(arg);
  if (!*dev) {
    return AVERROR(ENOMEM);
  }
  if ((opts_str = strchr(*dev, ','))) {
    *(opts_str++) = '\0';
    int ret = av_dict_parse_string(opts, opts_str, "=", ":", 0);
    if (opts_str[0] && ret < 0) {
      av_freep(dev);
      return ret;
    }
  }
  return 0;
}
}  // namespace

void show_sources(const std::string& device) {
  const char* arg = common::utils::c_strornull(device);
  AVInputFormat* fmt = NULL;
  char* dev = NULL;
  AVDictionary* opts = NULL;
  int ret = show_sinks_sources_parse_arg(arg, &dev, &opts);
  if (ret < 0) {
    av_dict_free(&opts);
    av_free(dev);
    return;
  }

  do {
    fmt = av_input_audio_device_next(fmt);
    if (fmt) {
      if (!strcmp(fmt->name, "lavfi")) {
        continue;  // it's pointless to probe lavfi
      }
      if (dev && !av_match_name(dev, fmt->name)) {
        continue;
      }
      print_device_sources(fmt, opts);
    }
  } while (fmt);
  do {
    fmt = av_input_video_device_next(fmt);
    if (fmt) {
      if (dev && !av_match_name(dev, fmt->name))
        continue;
      print_device_sources(fmt, opts);
    }
  } while (fmt);

  av_dict_free(&opts);
  av_free(dev);
}

void show_sinks(const std::string& device) {
  const char* arg = common::utils::c_strornull(device);
  AVOutputFormat* fmt = NULL;
  char* dev = NULL;
  AVDictionary* opts = NULL;
  int ret = show_sinks_sources_parse_arg(arg, &dev, &opts);
  if (ret < 0) {
    av_dict_free(&opts);
    av_free(dev);
    return;
  }

  do {
    fmt = av_output_audio_device_next(fmt);
    if (fmt) {
      if (dev && !av_match_name(dev, fmt->name))
        continue;
      print_device_sinks(fmt, opts);
    }
  } while (fmt);
  do {
    fmt = av_output_video_device_next(fmt);
    if (fmt) {
      if (dev && !av_match_name(dev, fmt->name))
        continue;
      print_device_sinks(fmt, opts);
    }
  } while (fmt);

  av_dict_free(&opts);
  av_free(dev);
}

#endif
