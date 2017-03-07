#pragma once

#include <inttypes.h>  // for PRIx64=
#include <stdint.h>    // for int64_t
#include <stdio.h>     // for snprintf
#include <ostream>     // for operator<<, basic_ostream
#include <string>      // for operator<<, string

#include <common/convert2string.h>  // for ConvertFromString
#include <common/logger.h>          // for COMPACT_LOG_WARNING, etc
#include <common/macros.h>          // for DISALLOW_COPY_AND_ASSIGN

#include "ffmpeg_config.h"  // for CONFIG_AVDEVICE, etc

extern "C" {
#include <libavcodec/avcodec.h>    // for AVCodec, AVCodecID
#include <libavformat/avformat.h>  // for AVFormatContext, AVStream
#include <libavutil/attributes.h>  // for av_noreturn
#include <libavutil/avutil.h>      // for av_get_media_type_string
#include <libavutil/channel_layout.h>
#include <libavutil/dict.h>       // for AVDictionary
#include <libavutil/log.h>        // for AVClass
#include <libavutil/pixdesc.h>    // for av_get_pix_fmt_name
#include <libavutil/samplefmt.h>  // for av_get_sample_fmt_name
}

#define HAS_ARG 0x0001
#define OPT_BOOL 0x0002
#define OPT_EXPERT 0x0004
#define OPT_STRING 0x0008
#define OPT_VIDEO 0x0010
#define OPT_AUDIO 0x0020
#define OPT_INT 0x0080
#define OPT_FLOAT 0x0100
#define OPT_SUBTITLE 0x0200
#define OPT_INT64 0x0400
#define OPT_EXIT 0x0800
#define OPT_DATA 0x1000

#define OPT_TIME 0x10000
#define OPT_DOUBLE 0x20000
#define OPT_INPUT 0x40000
#define OPT_OUTPUT 0x80000

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
 * Wraps exit with a program-specific cleanup routine.
 */
void exit_program(int ret) av_noreturn;

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
bool parse_number(const std::string& number_str, int type, T min, T max, T* result) {
  if (number_str.empty()) {
    WARNING_LOG() << "Can't parse value(number) invalid arguments!";
    return false;
  }

  T lresult = common::ConvertFromString<T>(number_str);
  if (lresult < min || lresult > max) {
    WARNING_LOG() << "The value for " << number_str << " was " << lresult << " which is not within "
                  << min << " - " << max;
    return false;
  } else if (type == OPT_INT64 && static_cast<int64_t>(lresult) != lresult) {
    WARNING_LOG() << "Expected int64 for " << number_str << " but found: " << lresult;
    return false;
  } else if (type == OPT_INT && static_cast<int>(lresult) != lresult) {
    WARNING_LOG() << "Expected int for " << number_str << " but found: " << lresult;
    return false;
  } else {
    if (result) {
      *result = lresult;
    }
    return true;
  }
}

bool parse_time(const std::string& time_str, bool is_duration, int64_t* result);

typedef struct OptionDef {
  const char* name;
  int flags;
  union {
    void* dst_ptr;
    int (*func_arg)(const char*, const char*, DictionaryOptions*);
  } u;
  const char* help;
  const char* argname;
} OptionDef;

/**
 * Print help for all options matching specified flags.
 *
 * @param options a list of options
 * @param msg title of this group. Only printed if at least one option matches.
 * @param req_flags print only options which have all those flags set.
 * @param rej_flags don't print options which have any of those flags set.
 * @param alt_flags print only options that have at least one of those flags set
 */
void show_help_options(const OptionDef* options,
                       const char* msg,
                       int req_flags,
                       int rej_flags,
                       int alt_flags);

/**
 * Show help for all options with given flags in class and all its
 * children.
 */
void show_help_children(const AVClass* cl, int flags);

/**
 * Per-fftool specific help handler. Implemented in each
 * fftool, called by show_help().
 */
void show_help_default(const char* opt, const char* arg);

/**
 * Generic -h handler common to all fftools.
 */
int show_help(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Parse the command line arguments.
 *
 * @param argc   number of command line arguments
 * @param argv   values of command line arguments
 * @param options Array with the definitions required to interpret every
 * option of the form: -option_name [argument]
 * @param optctx an opaque options context
 * @param parse_arg_function Name of the function called to process every
 * argument without a leading option name flag. NULL if such arguments do
 * not have to be processed.
 */
void parse_options(int argc, char** argv, const OptionDef* options, DictionaryOptions* dopt);

/**
 * Parse one given option.
 *
 * @return on success 1 if arg was consumed, 0 otherwise; negative number on error
 */
int parse_option(const char* opt,
                 const char* arg,
                 const OptionDef* options,
                 DictionaryOptions* dopt);

/**
 * Find the '-loglevel' option in the command line args and apply it.
 */
void parse_loglevel(int argc, char** argv, const OptionDef* options);

/**
 * Return index of option opt in argv or 0 if not found.
 */
int locate_option(int argc, char** argv, const OptionDef* options, const char* optname);

/**
 * Check if the given stream matches a stream specifier.
 *
 * @param s  Corresponding format context.
 * @param st Stream from s to be checked.
 * @param spec A stream specifier of the [v|a|s|d]:[\<stream index\>] form.
 *
 * @return 1 if the stream matches, 0 if it doesn't, <0 on error
 */
int check_stream_specifier(AVFormatContext* s, AVStream* st, const char* spec);

/**
 * Filter out options for given codec.
 *
 * Create a new options dictionary containing only the options from
 * opts which apply to the codec with ID codec_id.
 *
 * @param opts     dictionary to place options in
 * @param codec_id ID of the codec that should be filtered for
 * @param s Corresponding format context.
 * @param st A stream from s for which the options should be filtered.
 * @param codec The particular codec for which the options should be filtered.
 *              If null, the default one is looked up according to the codec id.
 * @return a pointer to the created dictionary
 */
AVDictionary* filter_codec_opts(AVDictionary* opts,
                                enum AVCodecID codec_id,
                                AVFormatContext* s,
                                AVStream* st,
                                AVCodec* codec);

/**
 * Setup AVCodecContext options for avformat_find_stream_info().
 *
 * Create an array of dictionaries, one dictionary for each stream
 * contained in s.
 * Each dictionary will contain the options from codec_opts which can
 * be applied to the corresponding stream codec context.
 *
 * @return pointer to the created array of dictionaries, NULL if it
 * cannot be created
 */
AVDictionary** setup_find_stream_info_opts(AVFormatContext* s, AVDictionary* codec_opts);

/**
 * Print the program banner to stderr. The banner contents depend on the
 * current version of the repository and of the libav* libraries used by
 * the program.
 */
void show_banner(int argc, char** argv, const OptionDef* options);

/**
 * Print the version of the program to stdout. The version message
 * depends on the current versions of the repository and of the libav*
 * libraries.
 * This option processing function does not utilize the arguments.
 */
int show_version(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Print the build configuration of the program to stdout. The contents
 * depend on the definition of FFMPEG_CONFIGURATION.
 * This option processing function does not utilize the arguments.
 */
int show_buildconf(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Print the license of the program to stdout. The license depends on
 * the license of the libraries compiled into the program.
 * This option processing function does not utilize the arguments.
 */
int show_license(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Print a listing containing all the formats supported by the
 * program (including devices).
 * This option processing function does not utilize the arguments.
 */
int show_formats(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Print a listing containing all the devices supported by the
 * program.
 * This option processing function does not utilize the arguments.
 */
int show_devices(const char* opt, const char* arg, DictionaryOptions* dopt);

#if CONFIG_AVDEVICE
/**
 * Print a listing containing autodetected sinks of the output device.
 * Device name with options may be passed as an argument to limit results.
 */
int show_sinks(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Print a listing containing autodetected sources of the input device.
 * Device name with options may be passed as an argument to limit results.
 */
int show_sources(const char* opt, const char* arg, DictionaryOptions* dopt);
#endif

/**
 * Print a listing containing all the codecs supported by the
 * program.
 * This option processing function does not utilize the arguments.
 */
int show_codecs(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Print a listing containing all the decoders supported by the
 * program.
 */
int show_decoders(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Print a listing containing all the encoders supported by the
 * program.
 */
int show_encoders(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Print a listing containing all the filters supported by the
 * program.
 * This option processing function does not utilize the arguments.
 */
int show_filters(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Print a listing containing all the bit stream filters supported by the
 * program.
 * This option processing function does not utilize the arguments.
 */
int show_bsfs(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Print a listing containing all the protocols supported by the
 * program.
 * This option processing function does not utilize the arguments.
 */
int show_protocols(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Print a listing containing all the pixel formats supported by the
 * program.
 * This option processing function does not utilize the arguments.
 */
int show_pix_fmts(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Print a listing containing all the standard channel layouts supported by
 * the program.
 * This option processing function does not utilize the arguments.
 */
int show_layouts(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Print a listing containing all the sample formats supported by the
 * program.
 */
int show_sample_fmts(const char* opt, const char* arg, DictionaryOptions* dopt);

/**
 * Print a listing containing all the color names and values recognized
 * by the program.
 */
int show_colors(const char* opt, const char* arg, DictionaryOptions* dopt);

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

double get_rotation(AVStream* st);
