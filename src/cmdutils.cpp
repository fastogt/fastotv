#include "cmdutils.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

/* Include only the enabled headers since some compilers (namely, Sun
   Studio) will not omit unused inline functions and create undefined
   references to libraries that are not being built. */

extern "C" {
//#include "compat/va_copy.h"
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavdevice/avdevice.h>
//#include <libavresample/avresample.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
//#include <libpostproc/postprocess.h>
#include <libavutil/avassert.h>
#include <libavutil/avstring.h>
#include <libavutil/bprint.h>
#include <libavutil/display.h>
#include <libavutil/mathematics.h>
#include <libavutil/imgutils.h>
//#include <libavutil/libm.h>
#include <libavutil/parseutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/eval.h>
#include <libavutil/dict.h>
#include <libavutil/opt.h>
#include <libavutil/cpu.h>
#include <libavutil/ffversion.h>
#include <libavutil/version.h>
#if CONFIG_NETWORK
#include "libavformat/network.h"
#endif
}
#if HAVE_SYS_RESOURCE_H
#include <sys/time.h>
#include <sys/resource.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

#include <vector>
#include <algorithm>

namespace {

FILE* report_file;
int report_file_level = AV_LOG_DEBUG;
bool warned_cfg = false;

bool compare_codec_desc(const AVCodecDescriptor* da, const AVCodecDescriptor* db) {
  return (da)->type != (db)->type ? FFDIFFSIGN((da)->type, (db)->type)
                                  : strcmp((da)->name, (db)->name);
}

bool is_device(const AVClass* avclass) {
  if (!avclass) {
    return false;
  }
  return AV_IS_INPUT_DEVICE(avclass->category) || AV_IS_OUTPUT_DEVICE(avclass->category);
}

#if CONFIG_AVFILTER
void show_help_filter(const char* name) {
#if CONFIG_AVFILTER
  if (!name) {
    av_log(NULL, AV_LOG_ERROR, "No filter name specified.\n");
    return;
  }

  const AVFilter* f = avfilter_get_by_name(name);
  if (!f) {
    av_log(NULL, AV_LOG_ERROR, "Unknown filter '%s'.\n", name);
    return;
  }

  printf("Filter %s\n", f->name);
  if (f->description) {
    printf("  %s\n", f->description);
  }

  if (f->flags & AVFILTER_FLAG_SLICE_THREADS) {
    printf("    slice threading supported\n");
  }

  printf("    Inputs:\n");
  int count = avfilter_pad_count(f->inputs);
  for (int i = 0; i < count; i++) {
    printf("       #%d: %s (%s)\n", i, avfilter_pad_get_name(f->inputs, i),
           media_type_string(avfilter_pad_get_type(f->inputs, i)));
  }
  if (f->flags & AVFILTER_FLAG_DYNAMIC_INPUTS) {
    printf("        dynamic (depending on the options)\n");
  } else if (!count) {
    printf("        none (source filter)\n");
  }

  printf("    Outputs:\n");
  count = avfilter_pad_count(f->outputs);
  for (int i = 0; i < count; i++) {
    printf("       #%d: %s (%s)\n", i, avfilter_pad_get_name(f->outputs, i),
           media_type_string(avfilter_pad_get_type(f->outputs, i)));
  }
  if (f->flags & AVFILTER_FLAG_DYNAMIC_OUTPUTS) {
    printf("        dynamic (depending on the options)\n");
  } else if (!count) {
    printf("        none (sink filter)\n");
  }

  if (f->priv_class) {
    show_help_children(f->priv_class, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_FILTERING_PARAM |
                                          AV_OPT_FLAG_AUDIO_PARAM);
  }
  if (f->flags & AVFILTER_FLAG_SUPPORT_TIMELINE) {
    printf("This filter has support for timeline through the 'enable' option.\n");
  }
#else
  av_log(NULL, AV_LOG_ERROR,
         "Build without libavfilter; "
         "can not to satisfy request\n");
#endif
}
#endif

char get_media_type_char(enum AVMediaType type) {
  switch (type) {
    case AVMEDIA_TYPE_VIDEO:
      return 'V';
    case AVMEDIA_TYPE_AUDIO:
      return 'A';
    case AVMEDIA_TYPE_DATA:
      return 'D';
    case AVMEDIA_TYPE_SUBTITLE:
      return 'S';
    case AVMEDIA_TYPE_ATTACHMENT:
      return 'T';
    default:
      return '?';
  }
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
  std::sort(lcodecs.begin(), lcodecs.end(), &compare_codec_desc);
  *rcodecs = lcodecs;
  return true;
}

const OptionDef* find_option(const OptionDef* po, const char* name) {
  const char* p = strchr(name, ':');
  size_t len = p ? (p - name) : strlen(name);

  while (po->name) {
    if (strncmp(name, po->name, len) == 0 && strlen(po->name) == len) {
      break;
    }
    po++;
  }
  return po;
}

const AVOption* opt_find(void* obj,
                         const char* name,
                         const char* unit,
                         int opt_flags,
                         int search_flags) {
  const AVOption* o = av_opt_find(obj, name, unit, opt_flags, search_flags);
  if (o && !o->flags) {
    return NULL;
  }

  return o;
}

inline void prepare_app_arguments(int* argc_ptr, char*** argv_ptr) {
  UNUSED(argc_ptr);
  UNUSED(argv_ptr);
  /* nothing to do */
}

void log_callback_report(void* ptr, int level, const char* fmt, va_list vl) {
  va_list vl2;
  char line[1024];
  static int print_prefix = 1;

  va_copy(vl2, vl);
  av_log_default_callback(ptr, level, fmt, vl);
  av_log_format_line(ptr, level, fmt, vl2, line, sizeof(line), &print_prefix);
  va_end(vl2);
  if (report_file_level >= level) {
    fputs(line, report_file);
    fflush(report_file);
  }
}

void expand_filename_template(AVBPrint* bp, const char* temp, struct tm* tm) {
  int c;

  while ((c = *(temp++))) {
    if (c == '%') {
      if (!(c = *(temp++))) {
        break;
      }
      switch (c) {
        case 'p':
          av_bprintf(bp, "%s", PROJECT_NAME_TITLE);
          break;
        case 't':
          av_bprintf(bp, "%04d%02d%02d-%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1,
                     tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
          break;
        case '%':
          av_bprint_chars(bp, c, 1);
          break;
      }
    } else {
      av_bprint_chars(bp, c, 1);
    }
  }
}

int init_report() {
  if (report_file) { /* already opened */
    return 0;
  }
  time_t now;
  time(&now);
  struct tm* tm = localtime(&now);

  char* filename_template = NULL;
  AVBPrint filename;
  av_bprint_init(&filename, 0, 1);
  const char* chr = static_cast<const char*>(av_x_if_null(filename_template, "%p-%t.log"));
  expand_filename_template(&filename, chr, tm);
  av_free(filename_template);
  if (!av_bprint_is_complete(&filename)) {
    av_log(NULL, AV_LOG_ERROR, "Out of memory building report file name\n");
    return AVERROR(ENOMEM);
  }

  report_file = fopen(filename.str, "w");
  if (!report_file) {
    int ret = AVERROR(errno);
    av_log(NULL, AV_LOG_ERROR, "Failed to open report \"%s\": %s\n", filename.str, strerror(errno));
    return ret;
  }
  av_log_set_callback(log_callback_report);
  av_log(NULL, AV_LOG_INFO,
         "%s started on %04d-%02d-%02d at %02d:%02d:%02d\n"
         "Report written to \"%s\"\n",
         PROJECT_NAME_TITLE, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
         tm->tm_min, tm->tm_sec, filename.str);
  av_bprint_finalize(&filename, NULL);
  return 0;
}

int write_option(const OptionDef* po, const char* opt, const char* arg, DictionaryOptions* dopt) {
  /* new-style options contain an offset into optctx, old-style address of
   * a global var*/
  void* dst = po->u.dst_ptr;

  if (po->flags & OPT_STRING) {
    char* str;
    str = av_strdup(arg);
    av_freep(dst);
    if (!str) {
      return AVERROR(ENOMEM);
    }
    *(char**)dst = str;
  } else if (po->flags & OPT_BOOL || po->flags & OPT_INT) {
    *(int*)dst = parse_number_or_die(opt, arg, OPT_INT64, INT_MIN, INT_MAX);
  } else if (po->flags & OPT_INT64) {
    *(int64_t*)dst = parse_number_or_die(opt, arg, OPT_INT64, INT64_MIN, INT64_MAX);
  } else if (po->flags & OPT_TIME) {
    *(int64_t*)dst = parse_time_or_die(opt, arg, 1);
  } else if (po->flags & OPT_FLOAT) {
    *(float*)dst = parse_number_or_die(opt, arg, OPT_FLOAT, -INFINITY, INFINITY);
  } else if (po->flags & OPT_DOUBLE) {
    *(double*)dst = parse_number_or_die(opt, arg, OPT_DOUBLE, -INFINITY, INFINITY);
  } else if (po->u.func_arg) {
    int ret = po->u.func_arg(opt, arg, dopt);
    if (ret < 0) {
      char err_str[AV_ERROR_MAX_STRING_SIZE] = {0};
      av_make_error_string(err_str, AV_ERROR_MAX_STRING_SIZE, ret);
      av_log(NULL, AV_LOG_ERROR, "Failed to set value '%s' for option '%s': %s\n", arg, opt,
             err_str);
      return ret;
    }
  }
  if (po->flags & OPT_EXIT) {
    exit_program(0);
  }

  return 0;
}

void dump_argument(const char* a) {
  const unsigned char* p;

  for (p = reinterpret_cast<const unsigned char*>(a); *p; p++) {
    if (!((*p >= '+' && *p <= ':') || (*p >= '@' && *p <= 'Z') || *p == '_' ||
          (*p >= 'a' && *p <= 'z'))) {
      break;
    }
  }
  if (!*p) {
    fputs(a, report_file);
    return;
  }
  fputc('"', report_file);
  for (p = reinterpret_cast<const unsigned char*>(a); *p; p++) {
    if (*p == '\\' || *p == '"' || *p == '$' || *p == '`') {
      fprintf(report_file, "\\%c", *p);
    } else if (*p < ' ' || *p > '~') {
      fprintf(report_file, "\\x%02x", *p);
    } else {
      fputc(*p, report_file);
    }
  }
  fputc('"', report_file);
}

#define INDENT 1
#define SHOW_VERSION 2
#define SHOW_CONFIG 4
#define SHOW_COPYRIGHT 8

#define PRINT_LIB_INFO(libname, LIBNAME, flags, level)                                           \
  if (CONFIG_##LIBNAME) {                                                                        \
    const char* indent = (flags & INDENT) ? "  " : "";                                           \
    if (flags & SHOW_VERSION) {                                                                  \
      unsigned int version = libname##_version();                                                \
      av_log(NULL, level, "%slib%-11s %2d.%3d.%3d / %2d.%3d.%3d\n", indent, #libname,            \
             LIB##LIBNAME##_VERSION_MAJOR, LIB##LIBNAME##_VERSION_MINOR,                         \
             LIB##LIBNAME##_VERSION_MICRO, AV_VERSION_MAJOR(version), AV_VERSION_MINOR(version), \
             AV_VERSION_MICRO(version));                                                         \
    }                                                                                            \
    if (flags & SHOW_CONFIG) {                                                                   \
      const char* cfg = libname##_configuration();                                               \
      if (strcmp(FFMPEG_CONFIGURATION, cfg)) {                                                   \
        if (!warned_cfg) {                                                                       \
          av_log(NULL, level, "%sWARNING: library configuration mismatch\n", indent);            \
          warned_cfg = true;                                                                     \
        }                                                                                        \
        av_log(NULL, level, "%s%-11s configuration: %s\n", indent, #libname, cfg);               \
      }                                                                                          \
    }                                                                                            \
  }

void print_all_libs_info(int flags, int level) {
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

void print_program_info(int flags, int level) {
  const char* indent = (flags & INDENT) ? "  " : "";

  av_log(NULL, level, "%s version " PROJECT_VERSION, PROJECT_NAME_TITLE);
  if (flags & SHOW_COPYRIGHT) {
    av_log(NULL, level, " " PROJECT_COPYRIGHT);
  }
  av_log(NULL, level, "\n");
  av_log(NULL, level, "%sbuilt with %s\n", indent, CC_IDENT);

  av_log(NULL, level,
         "%sFFMPEG version " FFMPEG_VERSION ", configuration: " FFMPEG_CONFIGURATION "\n", indent);
}

void print_buildconf(int flags, int level) {
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
  av_log(NULL, level, "\n%s FFMPEG configuration:\n", indent);
  while (splitconf != NULL) {
    av_log(NULL, level, "%s%s%s\n", indent, indent, splitconf);
    splitconf = strtok(NULL, "~");
  }
}

void show_help_demuxer(const char* name) {
  const AVInputFormat* fmt = av_find_input_format(name);

  if (!fmt) {
    av_log(NULL, AV_LOG_ERROR, "Unknown format '%s'.\n", name);
    return;
  }

  printf("Demuxer %s [%s]:\n", fmt->name, fmt->long_name);

  if (fmt->extensions) {
    printf("    Common extensions: %s.\n", fmt->extensions);
  }

  if (fmt->priv_class) {
    show_help_children(fmt->priv_class, AV_OPT_FLAG_DECODING_PARAM);
  }
}

void show_help_muxer(const char* name) {
  const AVCodecDescriptor* desc;
  const AVOutputFormat* fmt = av_guess_format(name, NULL, NULL);

  if (!fmt) {
    av_log(NULL, AV_LOG_ERROR, "Unknown format '%s'.\n", name);
    return;
  }

  printf("Muxer %s [%s]:\n", fmt->name, fmt->long_name);

  if (fmt->extensions) {
    printf("    Common extensions: %s.\n", fmt->extensions);
  }
  if (fmt->mime_type) {
    printf("    Mime type: %s.\n", fmt->mime_type);
  }
  if (fmt->video_codec != AV_CODEC_ID_NONE && (desc = avcodec_descriptor_get(fmt->video_codec))) {
    printf("    Default video codec: %s.\n", desc->name);
  }
  if (fmt->audio_codec != AV_CODEC_ID_NONE && (desc = avcodec_descriptor_get(fmt->audio_codec))) {
    printf("    Default audio codec: %s.\n", desc->name);
  }
  if (fmt->subtitle_codec != AV_CODEC_ID_NONE &&
      (desc = avcodec_descriptor_get(fmt->subtitle_codec))) {
    printf("    Default subtitle codec: %s.\n", desc->name);
  }

  if (fmt->priv_class) {
    show_help_children(fmt->priv_class, AV_OPT_FLAG_ENCODING_PARAM);
  }
}

const AVCodec* next_codec_for_id(enum AVCodecID id, const AVCodec* prev, int encoder) {
  while ((prev = av_codec_next(prev))) {
    if (prev->id == id && (encoder ? av_codec_is_encoder(prev) : av_codec_is_decoder(prev))) {
      return prev;
    }
  }
  return NULL;
}

void print_codecs_for_id(enum AVCodecID id, int encoder) {
  const AVCodec* codec = NULL;

  printf(" (%s: ", encoder ? "encoders" : "decoders");

  while ((codec = next_codec_for_id(id, codec, encoder))) {
    printf("%s ", codec->name);
  }

  printf(")");
}

void print_codecs(bool encoder) {
  printf(
      "%s:\n"
      " V..... = Video\n"
      " A..... = Audio\n"
      " S..... = Subtitle\n"
      " .F.... = Frame-level multithreading\n"
      " ..S... = Slice-level multithreading\n"
      " ...X.. = Codec is experimental\n"
      " ....B. = Supports draw_horiz_band\n"
      " .....D = Supports direct rendering method 1\n"
      " ------\n",
      encoder ? "Encoders" : "Decoders");

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
      if (strcmp(codec->name, desc->name))
        printf(" (codec %s)", desc->name);

      printf("\n");
    }
  }
}

int show_formats_devices(const char* opt,
                         const char* arg,
                         DictionaryOptions* dopt,
                         bool device_only) {
  UNUSED(dopt);
  UNUSED(opt);
  UNUSED(arg);

  AVInputFormat* ifmt = NULL;
  AVOutputFormat* ofmt = NULL;
  printf(
      "%s\n"
      " D. = Demuxing supported\n"
      " .E = Muxing supported\n"
      " --\n",
      device_only ? "Devices:" : "File formats:");
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

    printf(" %s%s %-15s %s\n", decode ? "D" : " ", encode ? "E" : " ", name,
           long_name ? long_name : " ");
  }
  return SUCCESS_RESULT_VALUE;
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

  printf("%s %s [%s]:\n", encoder ? "Encoder" : "Decoder", c->name,
         c->long_name ? c->long_name : "");

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
  if (c->capabilities &
      (AV_CODEC_CAP_FRAME_THREADS | AV_CODEC_CAP_SLICE_THREADS | AV_CODEC_CAP_AUTO_THREADS)) {
    printf("threads ");
  }
  if (!c->capabilities) {
    printf("none");
  }
  printf("\n");

  if (c->type == AVMEDIA_TYPE_VIDEO || c->type == AVMEDIA_TYPE_AUDIO) {
    printf("    Threading capabilities: ");
    switch (c->capabilities &
            (AV_CODEC_CAP_FRAME_THREADS | AV_CODEC_CAP_SLICE_THREADS | AV_CODEC_CAP_AUTO_THREADS)) {
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
  PRINT_CODEC_SUPPORTED(c, pix_fmts, enum AVPixelFormat, "pixel formats", AV_PIX_FMT_NONE,
                        GET_PIX_FMT_NAME);
  PRINT_CODEC_SUPPORTED(c, supported_samplerates, int, "sample rates", 0, GET_SAMPLE_RATE_NAME);
  PRINT_CODEC_SUPPORTED(c, sample_fmts, enum AVSampleFormat, "sample formats", AV_SAMPLE_FMT_NONE,
                        GET_SAMPLE_FMT_NAME);
  PRINT_CODEC_SUPPORTED(c, channel_layouts, uint64_t, "channel layouts", 0, GET_CH_LAYOUT_DESC);

  if (c->priv_class) {
    show_help_children(c->priv_class, AV_OPT_FLAG_ENCODING_PARAM | AV_OPT_FLAG_DECODING_PARAM);
  }
}

void show_help_codec(const char* name, int encoder) {
  const AVCodecDescriptor* desc;

  if (!name) {
    av_log(NULL, AV_LOG_ERROR, "No codec name specified.\n");
    return;
  }

  const AVCodec* codec =
      encoder ? avcodec_find_encoder_by_name(name) : avcodec_find_decoder_by_name(name);

  if (codec) {
    print_codec(codec);
  } else if ((desc = avcodec_descriptor_get_by_name(name))) {
    bool printed = false;

    while ((codec = next_codec_for_id(desc->id, codec, encoder))) {
      printed = true;
      print_codec(codec);
    }

    if (!printed) {
      av_log(NULL, AV_LOG_ERROR,
             "Codec '%s' is known to FFmpeg, "
             "but no %s for it are available. FFmpeg might need to be "
             "recompiled with additional external libraries.\n",
             name, encoder ? "encoders" : "decoders");
    }
  } else {
    av_log(NULL, AV_LOG_ERROR, "Codec '%s' is not recognized by FFmpeg.\n", name);
  }
}

int opt_loglevel_inner(const char* opt, const char* arg, char* argv) {
  UNUSED(argv);
  UNUSED(opt);

  const struct {
    const char* name;
    int level;
  } log_levels[] = {
      {"quiet", AV_LOG_QUIET},     {"panic", AV_LOG_PANIC},     {"fatal", AV_LOG_FATAL},
      {"error", AV_LOG_ERROR},     {"warning", AV_LOG_WARNING}, {"info", AV_LOG_INFO},
      {"verbose", AV_LOG_VERBOSE}, {"debug", AV_LOG_DEBUG},     {"trace", AV_LOG_TRACE},
  };

  int flags = av_log_get_flags();
  const char* tail = strstr(arg, "repeat");
  if (tail) {
    flags &= ~AV_LOG_SKIP_REPEATED;
  } else {
    flags |= AV_LOG_SKIP_REPEATED;
  }

  av_log_set_flags(flags);
  if (tail == arg) {
    arg += 6 + (arg[6] == '+');
  }
  if (tail && !*arg) {
    return 0;
  }

  for (size_t i = 0; i < FF_ARRAY_ELEMS(log_levels); i++) {
    if (!strcmp(log_levels[i].name, arg)) {
      av_log_set_level(log_levels[i].level);
      return 0;
    }
  }

  char* copy_tail = av_strdup(tail);
  if (!copy_tail) {
    return AVERROR(ENOMEM);
  }

  int level = strtol(arg, &copy_tail, 10);
  if (*copy_tail) {
    av_log(NULL, AV_LOG_FATAL,
           "Invalid loglevel \"%s\". "
           "Possible levels are numbers or:\n",
           arg);
    for (size_t i = 0; i < FF_ARRAY_ELEMS(log_levels); i++) {
      av_log(NULL, AV_LOG_FATAL, "\"%s\"\n", log_levels[i].name);
    }
    exit_program(1);
  }
  av_freep(&copy_tail);
  av_log_set_level(level);
  return 0;
}
}  // namespace

DictionaryOptions::DictionaryOptions()
    : sws_dict(NULL), swr_opts(NULL), format_opts(NULL), codec_opts(NULL) {
  av_dict_set(&sws_dict, "flags", "bicubic", 0);
}

DictionaryOptions::~DictionaryOptions() {
  av_dict_free(&swr_opts);
  av_dict_free(&sws_dict);
  av_dict_free(&format_opts);
  av_dict_free(&codec_opts);
}

void log_callback_help(void* ptr, int level, const char* fmt, va_list vl) {
  UNUSED(ptr);
  UNUSED(level);

  vfprintf(stdout, fmt, vl);
}

void init_dynload(void) {
#ifdef _WIN32
  /* Calling SetDllDirectory with the empty string (but not NULL) removes the
   * current working directory from the DLL search path as a security pre-caution. */
  SetDllDirectory("");
#endif
}

void exit_program(int ret) {
  exit(ret);
}

double parse_number_or_die(const char* context,
                           const char* numstr,
                           int type,
                           double min,
                           double max) {
  char* tail;
  const char* error;
  double d = av_strtod(numstr, &tail);
  if (*tail) {
    error = "Expected number for %s but found: %s\n";
  } else if (d < min || d > max) {
    error = "The value for %s was %s which is not within %f - %f\n";
  } else if (type == OPT_INT64 && (int64_t)d != d) {
    error = "Expected int64 for %s but found %s\n";
  } else if (type == OPT_INT && (int)d != d) {
    error = "Expected int for %s but found %s\n";
  } else {
    return d;
  }
  av_log(NULL, AV_LOG_FATAL, error, context, numstr, min, max);
  exit_program(1);
  return 0;
}

int64_t parse_time_or_die(const char* context, const char* timestr, int is_duration) {
  int64_t us;
  if (av_parse_time(&us, timestr, is_duration) < 0) {
    av_log(NULL, AV_LOG_FATAL, "Invalid %s specification for %s: %s\n",
           is_duration ? "duration" : "date", context, timestr);
    exit_program(1);
  }
  return us;
}

void show_help_options(const OptionDef* options,
                       const char* msg,
                       int req_flags,
                       int rej_flags,
                       int alt_flags) {
  const OptionDef* po;
  bool first = true;
  for (po = options; po->name; po++) {
    char buf[64];

    if (((po->flags & req_flags) != req_flags) || (alt_flags && !(po->flags & alt_flags)) ||
        (po->flags & rej_flags)) {
      continue;
    }

    if (first) {
      printf("%s\n", msg);
      first = false;
    }
    av_strlcpy(buf, po->name, sizeof(buf));
    if (po->argname) {
      av_strlcat(buf, " ", sizeof(buf));
      av_strlcat(buf, po->argname, sizeof(buf));
    }
    printf("-%-17s  %s\n", buf, po->help);
  }
  printf("\n");
}

void show_help_children(const AVClass* cl, int flags) {
  if (cl->option) {
    av_opt_show2(&cl, NULL, flags, 0);
    printf("\n");
  }

  const AVClass* child = NULL;
  while ((child = av_opt_child_class_next(cl, child))) {
    show_help_children(child, flags);
  }
}

int parse_option(const char* opt,
                 const char* arg,
                 const OptionDef* options,
                 DictionaryOptions* dopt) {
  const OptionDef* po = find_option(options, opt);
  if (!po->name && opt[0] == 'n' && opt[1] == 'o') {
    /* handle 'no' bool option */
    po = find_option(options, opt + 2);
    if ((po->name && (po->flags & OPT_BOOL))) {
      arg = "0";
    }
  } else if (po->flags & OPT_BOOL) {
    arg = "1";
  }

  if (!po->name) {
    po = find_option(options, "default");
  }
  if (!po->name) {
    av_log(NULL, AV_LOG_ERROR, "Unrecognized option '%s'\n", opt);
    return AVERROR(EINVAL);
  }
  if (po->flags & HAS_ARG && !arg) {
    av_log(NULL, AV_LOG_ERROR, "Missing argument for option '%s'\n", opt);
    return AVERROR(EINVAL);
  }

  int ret = write_option(po, opt, arg, dopt);
  if (ret < 0) {
    return ret;
  }

  return !!(po->flags & HAS_ARG);
}

void parse_options(int argc, char** argv, const OptionDef* options, DictionaryOptions* dopt) {
  bool handleoptions = true;

  /* perform system-dependent conversions for arguments list */
  prepare_app_arguments(&argc, &argv);

  /* parse options */
  int optindex = 1;
  while (optindex < argc) {
    const char* opt = argv[optindex++];

    if (handleoptions && opt[0] == '-' && opt[1] != '\0') {
      if (opt[1] == '-' && opt[2] == '\0') {
        handleoptions = false;
        continue;
      }
      opt++;

      int ret = parse_option(opt, argv[optindex], options, dopt);
      if (ret < 0) {
        exit_program(1);
      }
      optindex += ret;
    }
  }
}

int locate_option(int argc, char** argv, const OptionDef* options, const char* optname) {
  for (int i = 1; i < argc; i++) {
    const char* cur_opt = argv[i];

    if (*cur_opt++ != '-') {
      continue;
    }

    const OptionDef* po = find_option(options, cur_opt);
    if (!po->name && cur_opt[0] == 'n' && cur_opt[1] == 'o') {
      po = find_option(options, cur_opt + 2);
    }

    if ((!po->name && !strcmp(cur_opt, optname)) || (po->name && !strcmp(optname, po->name))) {
      return i;
    }

    if (!po->name || po->flags & HAS_ARG) {
      i++;
    }
  }
  return 0;
}

void parse_loglevel(int argc, char** argv, const OptionDef* options) {
  int idx = locate_option(argc, argv, options, "loglevel");
  if (!idx) {
    idx = locate_option(argc, argv, options, "v");
  }
  if (idx && argv[idx + 1]) {
    opt_loglevel_inner(NULL, "loglevel", argv[idx + 1]);
  }
  idx = locate_option(argc, argv, options, "report");
  if (idx) {
    init_report();
    if (report_file) {
      int i;
      fprintf(report_file, "Command line:\n");
      for (i = 0; i < argc; i++) {
        dump_argument(argv[i]);
        fputc(i < argc - 1 ? ' ' : '\n', report_file);
      }
      fflush(report_file);
    }
  }
}

#define FLAGS \
  (o->type == AV_OPT_TYPE_FLAGS && (arg[0] == '-' || arg[0] == '+')) ? AV_DICT_APPEND : 0
int opt_default(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  const AVOption* o;
  bool consumed = false;
  char opt_stripped[128];
  const char* p;
  const AVClass *cc = avcodec_get_class(), *fc = avformat_get_class();
#if CONFIG_AVRESAMPLE
  const AVClass* rc = avresample_get_class();
#endif
#if CONFIG_SWSCALE
  const AVClass* sc = sws_get_class();
#endif
#if CONFIG_SWRESAMPLE
  const AVClass* swr_class = swr_get_class();
#endif

  if (!strcmp(opt, "debug") || !strcmp(opt, "fdebug")) {
    av_log_set_level(AV_LOG_DEBUG);
  }

  if (!(p = strchr(opt, ':'))) {
    p = opt + strlen(opt);
  }
  av_strlcpy(opt_stripped, opt, FFMIN(sizeof(opt_stripped), p - opt + 1));

  if ((o = opt_find(&cc, opt_stripped, NULL, 0, AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ)) ||
      ((opt[0] == 'v' || opt[0] == 'a' || opt[0] == 's') &&
       (o = opt_find(&cc, opt + 1, NULL, 0, AV_OPT_SEARCH_FAKE_OBJ)))) {
    av_dict_set(&dopt->codec_opts, opt, arg, FLAGS);
    consumed = true;
  }
  if ((o = opt_find(&fc, opt, NULL, 0, AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ))) {
    av_dict_set(&dopt->format_opts, opt, arg, FLAGS);
    if (consumed) {
      av_log(NULL, AV_LOG_VERBOSE, "Routing option %s to both codec and muxer layer\n", opt);
    }
    consumed = true;
  }
#if CONFIG_SWSCALE
  if (!consumed &&
      (o = opt_find(&sc, opt, NULL, 0, AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ))) {
    struct SwsContext* sws = sws_alloc_context();
    int ret = av_opt_set(sws, opt, arg, 0);
    sws_freeContext(sws);
    if (!strcmp(opt, "srcw") || !strcmp(opt, "srch") || !strcmp(opt, "dstw") ||
        !strcmp(opt, "dsth") || !strcmp(opt, "src_format") || !strcmp(opt, "dst_format")) {
      av_log(NULL, AV_LOG_ERROR,
             "Directly using swscale dimensions/format options is not supported, please use the -s "
             "or -pix_fmt options\n");
      return AVERROR(EINVAL);
    }
    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Error setting option %s.\n", opt);
      return ret;
    }

    av_dict_set(&dopt->sws_dict, opt, arg, FLAGS);

    consumed = true;
  }
#else
  if (!consumed && !strcmp(opt, "sws_flags")) {
    av_log(NULL, AV_LOG_WARNING, "Ignoring %s %s, due to disabled swscale\n", opt, arg);
    consumed = true;
  }
#endif
#if CONFIG_SWRESAMPLE
  if (!consumed &&
      (o = opt_find(&swr_class, opt, NULL, 0, AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ))) {
    struct SwrContext* swr = swr_alloc();
    int ret = av_opt_set(swr, opt, arg, 0);
    swr_free(&swr);
    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Error setting option %s.\n", opt);
      return ret;
    }
    av_dict_set(&dopt->swr_opts, opt, arg, FLAGS);
    consumed = true;
  }
#endif
#if CONFIG_AVRESAMPLE
  if ((o = opt_find(&rc, opt, NULL, 0, AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ))) {
    av_dict_set(&resample_opts, opt, arg, FLAGS);
    consumed = 1;
  }
#endif

  if (consumed) {
    return 0;
  }
  return AVERROR_OPTION_NOT_FOUND;
}

int opt_cpuflags(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);

  unsigned flags = av_get_cpu_flags();
  int ret = av_parse_cpu_caps(&flags, arg);
  if (ret < 0) {
    return ret;
  }

  av_force_cpu_flags(flags);
  return 0;
}

int opt_loglevel(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  return opt_loglevel_inner(opt, arg, NULL);
}

int opt_report(const char* opt) {
  UNUSED(opt);

  return init_report();
}

int opt_max_alloc(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);

  char* tail = NULL;
  long max = strtol(arg, &tail, 10);
  if (*tail) {
    av_log(NULL, AV_LOG_FATAL, "Invalid max_alloc \"%s\".\n", arg);
    exit_program(1);
  }
  av_max_alloc(max);
  return 0;
}

void show_banner(int argc, char** argv, const OptionDef* options) {
  print_program_info(INDENT | SHOW_COPYRIGHT, AV_LOG_INFO);
  print_all_libs_info(INDENT | SHOW_CONFIG, AV_LOG_INFO);
  print_all_libs_info(INDENT | SHOW_VERSION, AV_LOG_INFO);
}

int show_version(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);
  UNUSED(arg);

  av_log_set_callback(log_callback_help);
  print_program_info(SHOW_COPYRIGHT, AV_LOG_INFO);
  print_all_libs_info(SHOW_VERSION, AV_LOG_INFO);
  return 0;
}

int show_buildconf(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);
  UNUSED(arg);

  av_log_set_callback(log_callback_help);
  print_buildconf(INDENT | 0, AV_LOG_INFO);

  return 0;
}

int show_license(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);
  UNUSED(arg);

  printf(
      "%s is free software; you can redistribute it and/or modify\n"
      "it under the terms of the GNU Lesser General Public License as published by\n"
      "the Free Software Foundation; either version 3 of the License, or\n"
      "(at your option) any later version.\n"
      "\n"
      "%s is distributed in the hope that it will be useful,\n"
      "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
      "GNU Lesser General Public License for more details.\n"
      "\n"
      "You should have received a copy of the GNU Lesser General Public License\n"
      "along with %s.  If not, see <http://www.gnu.org/licenses/>.\n",
      PROJECT_NAME_TITLE, PROJECT_NAME_TITLE, PROJECT_NAME_TITLE);

  return 0;
}

int show_formats(const char* opt, const char* arg, DictionaryOptions* dopt) {
  return show_formats_devices(opt, arg, dopt, false);
}

int show_devices(const char* opt, const char* arg, DictionaryOptions* dopt) {
  return show_formats_devices(opt, arg, dopt, true);
}

int show_codecs(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);
  UNUSED(arg);

  printf(
      "Codecs:\n"
      " D..... = Decoding supported\n"
      " .E.... = Encoding supported\n"
      " ..V... = Video codec\n"
      " ..A... = Audio codec\n"
      " ..S... = Subtitle codec\n"
      " ...I.. = Intra frame-only codec\n"
      " ....L. = Lossy compression\n"
      " .....S = Lossless compression\n"
      " -------\n");

  std::vector<const AVCodecDescriptor*> codecs;
  bool is_ok = get_codecs_sorted(&codecs);
  if (!is_ok) {
    return ERROR_RESULT_VALUE;
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
  return 0;
}

int show_decoders(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);
  UNUSED(arg);

  print_codecs(false);
  return 0;
}

int show_encoders(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);
  UNUSED(arg);

  print_codecs(true);
  return 0;
}

int show_bsfs(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);
  UNUSED(arg);

  const AVBitStreamFilter* bsf = NULL;
  void* opaque = NULL;

  printf("Bitstream filters:\n");
  while ((bsf = av_bsf_next(&opaque))) {
    printf("%s\n", bsf->name);
  }
  printf("\n");
  return 0;
}

int show_protocols(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);
  UNUSED(arg);

  void* opaque = NULL;
  const char* name;

  printf(
      "Supported file protocols:\n"
      "Input:\n");
  while ((name = avio_enum_protocols(&opaque, 0))) {
    printf("  %s\n", name);
  }
  printf("Output:\n");
  while ((name = avio_enum_protocols(&opaque, 1))) {
    printf("  %s\n", name);
  }
  return 0;
}

int show_filters(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);
  UNUSED(arg);

#if CONFIG_AVFILTER
  const AVFilter* filter = NULL;
  char descr[64];
  const AVFilterPad* pad;

  printf(
      "Filters:\n"
      "  T.. = Timeline support\n"
      "  .S. = Slice threading\n"
      "  ..C = Command support\n"
      "  A = Audio input/output\n"
      "  V = Video input/output\n"
      "  N = Dynamic number and/or type of input/output\n"
      "  | = Source or sink filter\n");
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
           (filter->flags & AVFILTER_FLAG_SLICE_THREADS) ? 'S' : '.',
           filter->process_command ? 'C' : '.', filter->name, descr, filter->description);
  }
#else
  printf("No filters available: libavfilter disabled\n");
#endif
  return 0;
}

int show_colors(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);
  UNUSED(arg);

  const char* name;
  const uint8_t* rgb;
  printf("%-32s #RRGGBB\n", "name");

  for (int i = 0; (name = av_get_known_color_name(i, &rgb)); i++) {
    printf("%-32s #%02x%02x%02x\n", name, rgb[0], rgb[1], rgb[2]);
  }

  return 0;
}

int show_pix_fmts(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);
  UNUSED(arg);

  const AVPixFmtDescriptor* pix_desc = NULL;

  printf(
      "Pixel formats:\n"
      "I.... = Supported Input  format for conversion\n"
      ".O... = Supported Output format for conversion\n"
      "..H.. = Hardware accelerated format\n"
      "...P. = Paletted format\n"
      "....B = Bitstream format\n"
      "FLAGS NAME            NB_COMPONENTS BITS_PER_PIXEL\n"
      "-----\n");

#if !CONFIG_SWSCALE
#define sws_isSupportedInput(x) 0
#define sws_isSupportedOutput(x) 0
#endif

  while ((pix_desc = av_pix_fmt_desc_next(pix_desc))) {
    enum AVPixelFormat pix_fmt = av_pix_fmt_desc_get_id(pix_desc);
    printf("%c%c%c%c%c %-16s       %d            %2d\n", sws_isSupportedInput(pix_fmt) ? 'I' : '.',
           sws_isSupportedOutput(pix_fmt) ? 'O' : '.',
           (pix_desc->flags & AV_PIX_FMT_FLAG_HWACCEL) ? 'H' : '.',
           (pix_desc->flags & AV_PIX_FMT_FLAG_PAL) ? 'P' : '.',
           (pix_desc->flags & AV_PIX_FMT_FLAG_BITSTREAM) ? 'B' : '.', pix_desc->name,
           pix_desc->nb_components, av_get_bits_per_pixel(pix_desc));
  }
  return 0;
}

int show_layouts(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);
  UNUSED(arg);

  uint64_t layout, j;
  const char *name, *descr;

  printf(
      "Individual channels:\n"
      "NAME           DESCRIPTION\n");
  for (int i = 0; i < 63; i++) {
    name = av_get_channel_name((uint64_t)1 << i);
    if (!name)
      continue;
    descr = av_get_channel_description((uint64_t)1 << i);
    printf("%-14s %s\n", name, descr);
  }
  printf(
      "\nStandard channel layouts:\n"
      "NAME           DECOMPOSITION\n");
  for (int i = 0; !av_get_standard_channel_layout(i, &layout, &name); i++) {
    if (name) {
      printf("%-14s ", name);
      for (j = 1; j; j <<= 1)
        if ((layout & j))
          printf("%s%s", (layout & (j - 1)) ? "+" : "", av_get_channel_name(j));
      printf("\n");
    }
  }
  return 0;
}

int show_sample_fmts(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);
  UNUSED(arg);

  for (int i = -1; i < AV_SAMPLE_FMT_NB; i++) {
    char fmt_str[128] = {0};
    const AVSampleFormat sample_fmt = static_cast<AVSampleFormat>(i);
    const char* str = av_get_sample_fmt_string(fmt_str, sizeof(fmt_str), sample_fmt);
    printf("%s\n", str);
  }
  return 0;
}

int show_help(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);

  av_log_set_callback(log_callback_help);
  char* topic = av_strdup(arg ? arg : "");
  if (!topic) {
    return AVERROR(ENOMEM);
  }
  char* par = strchr(topic, '=');
  if (par) {
    *par++ = 0;
  }

  if (!*topic) {
    show_help_default(topic, par);
  } else if (!strcmp(topic, "decoder")) {
    show_help_codec(par, 0);
  } else if (!strcmp(topic, "encoder")) {
    show_help_codec(par, 1);
  } else if (!strcmp(topic, "demuxer")) {
    show_help_demuxer(par);
  } else if (!strcmp(topic, "muxer")) {
    show_help_muxer(par);
#if CONFIG_AVFILTER
  } else if (!strcmp(topic, "filter")) {
    show_help_filter(par);
#endif
  } else {
    show_help_default(topic, par);
  }

  av_freep(&topic);
  return 0;
}

int check_stream_specifier(AVFormatContext* s, AVStream* st, const char* spec) {
  int ret = avformat_match_stream_specifier(s, st, spec);
  if (ret < 0) {
    av_log(s, AV_LOG_ERROR, "Invalid stream specifier: %s.\n", spec);
  }
  return ret;
}

AVDictionary* filter_codec_opts(AVDictionary* opts,
                                enum AVCodecID codec_id,
                                AVFormatContext* s,
                                AVStream* st,
                                AVCodec* codec) {
  AVDictionary* ret = NULL;
  AVDictionaryEntry* t = NULL;
  int flags = s->oformat ? AV_OPT_FLAG_ENCODING_PARAM : AV_OPT_FLAG_DECODING_PARAM;
  char prefix = 0;
  const AVClass* cc = avcodec_get_class();

  if (!codec) {
    codec = s->oformat ? avcodec_find_encoder(codec_id) : avcodec_find_decoder(codec_id);
  }

  switch (st->codecpar->codec_type) {
    case AVMEDIA_TYPE_VIDEO: {
      prefix = 'v';
      flags |= AV_OPT_FLAG_VIDEO_PARAM;
      break;
    }
    case AVMEDIA_TYPE_AUDIO: {
      prefix = 'a';
      flags |= AV_OPT_FLAG_AUDIO_PARAM;
      break;
    }
    case AVMEDIA_TYPE_SUBTITLE: {
      prefix = 's';
      flags |= AV_OPT_FLAG_SUBTITLE_PARAM;
      break;
    }
    default:
      break;
  }

  while ((t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX))) {
    char* p = strchr(t->key, ':');

    /* check stream specification in opt name */
    if (p)
      switch (check_stream_specifier(s, st, p + 1)) {
        case 1:
          *p = 0;
          break;
        case 0:
          continue;
        default:
          exit_program(1);
      }

    if (av_opt_find(&cc, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ) || !codec ||
        (codec->priv_class &&
         av_opt_find(&codec->priv_class, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ)))
      av_dict_set(&ret, t->key, t->value, 0);
    else if (t->key[0] == prefix &&
             av_opt_find(&cc, t->key + 1, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ))
      av_dict_set(&ret, t->key + 1, t->value, 0);

    if (p) {
      *p = ':';
    }
  }
  return ret;
}

AVDictionary** setup_find_stream_info_opts(AVFormatContext* s, AVDictionary* codec_opts) {
  if (!s->nb_streams) {
    return NULL;
  }

  AVDictionary** opts = static_cast<AVDictionary**>(av_mallocz_array(s->nb_streams, sizeof(*opts)));
  if (!opts) {
    av_log(NULL, AV_LOG_ERROR, "Could not alloc memory for stream options.\n");
    return NULL;
  }
  for (int i = 0; i < s->nb_streams; i++) {
    opts[i] =
        filter_codec_opts(codec_opts, s->streams[i]->codecpar->codec_id, s, s->streams[i], NULL);
  }
  return opts;
}

double get_rotation(AVStream* st) {
  AVDictionaryEntry* rotate_tag = av_dict_get(st->metadata, "rotate", NULL, 0);
  uint8_t* displaymatrix = av_stream_get_side_data(st, AV_PKT_DATA_DISPLAYMATRIX, NULL);
  double theta = 0;

  if (rotate_tag && *rotate_tag->value && strcmp(rotate_tag->value, "0")) {
    char* tail;
    theta = av_strtod(rotate_tag->value, &tail);
    if (*tail) {
      theta = 0;
    }
  }
  if (displaymatrix && !theta) {
    theta = -av_display_rotation_get((int32_t*)displaymatrix);
  }

  theta -= 360 * floor(theta / 360 + 0.9 / 360);

  if (fabs(theta - 90 * round(theta / 90)) > 2) {
    av_log(NULL, AV_LOG_WARNING,
           "Odd rotation angle.\n"
           "If you want to help, upload a sample "
           "of this file to ftp://upload.ffmpeg.org/incoming/ "
           "and contact the ffmpeg-devel mailing list. (ffmpeg-devel@ffmpeg.org)");
  }

  return theta;
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
    printf("%s %s [%s]\n", device_list->default_device == i ? "*" : " ",
           device_list->devices[i]->device_name, device_list->devices[i]->device_description);
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
    printf("%s %s [%s]\n", device_list->default_device == i ? "*" : " ",
           device_list->devices[i]->device_name, device_list->devices[i]->device_description);
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

int show_sources(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);

  AVInputFormat* fmt = NULL;
  char* dev = NULL;
  AVDictionary* opts = NULL;
  int error_level = av_log_get_level();

  av_log_set_level(AV_LOG_ERROR);

  int ret = show_sinks_sources_parse_arg(arg, &dev, &opts);
  if (ret < 0) {
    av_dict_free(&opts);
    av_free(dev);
    av_log_set_level(error_level);
    return ret;
  }

  do {
    fmt = av_input_audio_device_next(fmt);
    if (fmt) {
      if (!strcmp(fmt->name, "lavfi"))
        continue;  // it's pointless to probe lavfi
      if (dev && !av_match_name(dev, fmt->name))
        continue;
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
  av_log_set_level(error_level);
  return 0;
}

int show_sinks(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);

  AVOutputFormat* fmt = NULL;
  char* dev = NULL;
  AVDictionary* opts = NULL;
  int error_level = av_log_get_level();

  av_log_set_level(AV_LOG_ERROR);

  int ret = show_sinks_sources_parse_arg(arg, &dev, &opts);
  if (ret < 0) {
    av_dict_free(&opts);
    av_free(dev);
    av_log_set_level(error_level);
    return ret;
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
  av_log_set_level(error_level);
  return 0;
}

#endif
