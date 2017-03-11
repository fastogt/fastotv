#include <errno.h>   // for EINVAL
#include <signal.h>  // for signal, SIGINT, SIGTERM
#include <stdio.h>   // for printf
#include <stdlib.h>  // for exit, EXIT_FAILURE
#include <string.h>  // for strcmp, NULL, strchr
#include <limits>    // for numeric_limits
#include <ostream>   // for operator<<, basic_ostream
#include <string>    // for char_traits, string, etc

#include "ffmpeg_config.h"

extern "C" {
#include <libavcodec/avcodec.h>    // for av_lockmgr_register, etc
#include <libavdevice/avdevice.h>  // for avdevice_register_all
#if CONFIG_AVFILTER
#include <libavfilter/avfilter.h>  // for avfilter_get_class, etc
#endif
#include <libavformat/avformat.h>  // for av_find_input_format, etc
#include <libavutil/avutil.h>
#include <libavutil/error.h>  // for AVERROR
#include <libavutil/opt.h>    // for AV_OPT_FLAG_DECODING_PARAM, etc
}

#include <common/log_levels.h>  // for LEVEL_LOG, etc
#include <common/logger.h>      // for LogMessage, etc
#include <common/macros.h>      // for UNUSED, etc
#include <common/file_system.h>
#include <common/threads/types.h>

#include "cmdutils.h"          // for HAS_ARG, OPT_EXPERT, etc
#include "core/app_options.h"  // for AppOptions, ComplexOptions
#include "core/types.h"
#include "core/application/sdl2_application.h"

#include "ffmpeg_config.h"  // for CONFIG_AVFILTER, etc
#include "player.h"

#undef ERROR

namespace {
fasto::fastotv::core::AppOptions g_options;
fasto::fastotv::PlayerOptions g_player_options;
#if CONFIG_AVFILTER
int opt_add_vfilter(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);

  g_options.InitAvFilters(arg);
  return 0;
}
#endif

void sigterm_handler(int sig) {
  UNUSED(sig);

  exit(123);
}

int opt_frame_size(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);

  WARNING_LOG() << "Option -s is deprecated, use -video_size.";
  return opt_default("video_size", arg, dopt);
}

static int opt_width(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);

  if (!parse_number(arg, OPT_INT, 1, std::numeric_limits<int>::max(),
                    &g_player_options.screen_width)) {
    return ERROR_RESULT_VALUE;
  }
  return SUCCESS_RESULT_VALUE;
}

int opt_height(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);

  if (!parse_number(arg, OPT_INT, 1, std::numeric_limits<int>::max(),
                    &g_player_options.screen_height)) {
    return ERROR_RESULT_VALUE;
  }
  return SUCCESS_RESULT_VALUE;
}

int opt_frame_pix_fmt(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);

  WARNING_LOG() << "Option -pix_fmt is deprecated, use -pixel_format.";
  return opt_default("pixel_format", arg, dopt);
}

int opt_sync(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);

  if (!strcmp(arg, "audio")) {
    g_options.av_sync_type = fasto::fastotv::core::AV_SYNC_AUDIO_MASTER;
  } else if (!strcmp(arg, "video")) {
    g_options.av_sync_type = fasto::fastotv::core::AV_SYNC_VIDEO_MASTER;
  } else {
    ERROR_LOG() << "Unknown value for " << opt << ": " << arg;
    exit(1);
  }
  return 0;
}

int opt_codec(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);

  const char* spec = strchr(opt, ':');
  if (!spec) {
    ERROR_LOG() << "No media specifier was specified in '" << arg << "' in option '" << opt << "'";
    return AVERROR(EINVAL);
  }
  spec++;
  switch (spec[0]) {
    case 'a': {
      g_options.audio_codec_name = arg;
      break;
    }
    case 'v': {
      g_options.video_codec_name = arg;
      break;
    }
    default: {
      ERROR_LOG() << "Invalid media specifier '" << spec << "' in option '" << opt << "'";
      return AVERROR(EINVAL);
    }
  }
  return 0;
}

const OptionDef options[] = {
    {"L", OPT_EXIT, {.func_arg = show_license}, "show license"},
    {"h", OPT_EXIT, {.func_arg = show_help}, "show help", "topic"},
    {"?", OPT_EXIT, {.func_arg = show_help}, "show help", "topic"},
    {"help", OPT_EXIT, {.func_arg = show_help}, "show help", "topic"},
    {"-help", OPT_EXIT, {.func_arg = show_help}, "show help", "topic"},
    {"version", OPT_EXIT, {.func_arg = show_version}, "show version"},
    {"buildconf", OPT_EXIT, {.func_arg = show_buildconf}, "show build configuration"},
    {"formats", OPT_EXIT, {.func_arg = show_formats}, "show available formats"},
    {"devices", OPT_EXIT, {.func_arg = show_devices}, "show available devices"},
    {"codecs", OPT_EXIT, {.func_arg = show_codecs}, "show available codecs"},
    {"decoders", OPT_EXIT, {.func_arg = show_decoders}, "show available decoders"},
    {"encoders", OPT_EXIT, {.func_arg = show_encoders}, "show available encoders"},
    {"bsfs", OPT_EXIT, {.func_arg = show_bsfs}, "show available bit stream filters"},
    {"protocols", OPT_EXIT, {.func_arg = show_protocols}, "show available protocols"},
    {"filters", OPT_EXIT, {.func_arg = show_filters}, "show available filters"},
    {"pix_fmts", OPT_EXIT, {.func_arg = show_pix_fmts}, "show available pixel formats"},
    {"layouts", OPT_EXIT, {.func_arg = show_layouts}, "show standard channel layouts"},
    {"sample_fmts",
     OPT_EXIT,
     {.func_arg = show_sample_fmts},
     "show available audio sample formats"},
    {"colors", OPT_EXIT, {.func_arg = show_colors}, "show available color names"},
    {"loglevel", HAS_ARG, {.func_arg = opt_loglevel}, "set logging level", "loglevel"},
    {"v", HAS_ARG, {.func_arg = opt_loglevel}, "set logging level", "loglevel"},
#if CONFIG_AVDEVICE
    {"sources",
     OPT_EXIT | HAS_ARG,
     {.func_arg = show_sources},
     "list sources of the input device",
     "device"},
    {"sinks",
     OPT_EXIT | HAS_ARG,
     {.func_arg = show_sinks},
     "list sinks of the output device",
     "device"},
#endif
    {"x", HAS_ARG, {.func_arg = opt_width}, "force displayed width", "width"},
    {"y", HAS_ARG, {.func_arg = opt_height}, "force displayed height", "height"},
    {"s",
     HAS_ARG | OPT_VIDEO,
     {.func_arg = opt_frame_size},
     "set frame size (WxH or abbreviation)",
     "size"},
    {"fs", OPT_BOOL, {&g_player_options.is_full_screen}, "force full screen"},
    {"ast",
     OPT_STRING | HAS_ARG | OPT_EXPERT,
     {&g_options.wanted_stream_spec[AVMEDIA_TYPE_AUDIO]},
     "select desired audio stream",
     "stream_specifier"},
    {"vst",
     OPT_STRING | HAS_ARG | OPT_EXPERT,
     {&g_options.wanted_stream_spec[AVMEDIA_TYPE_VIDEO]},
     "select desired video stream",
     "stream_specifier"},
    {"bytes",
     OPT_INT | HAS_ARG,
     {&g_options.seek_by_bytes},
     "seek by bytes 0=off 1=on -1=auto",
     "val"},
    {"volume",
     OPT_INT | HAS_ARG,
     {&g_player_options.audio_volume},
     "set startup volume 0=min 100=max",
     "volume"},
    {"pix_fmt",
     HAS_ARG | OPT_EXPERT | OPT_VIDEO,
     {.func_arg = opt_frame_pix_fmt},
     "set pixel format",
     "format"},
    {"stats", OPT_BOOL | OPT_EXPERT, {&g_options.show_status}, "show status", ""},
    {"fast", OPT_BOOL | OPT_EXPERT, {&g_options.fast}, "non spec compliant optimizations", ""},
    {"genpts", OPT_BOOL | OPT_EXPERT, {&g_options.genpts}, "generate pts", ""},
    {"lowres", OPT_INT | HAS_ARG | OPT_EXPERT, {&g_options.lowres}, "", ""},
    {"sync",
     HAS_ARG | OPT_EXPERT,
     {.func_arg = opt_sync},
     "set audio-video sync. type (type=audio/video)",
     "type"},
    {"exitonkeydown",
     OPT_BOOL | OPT_EXPERT,
     {&g_player_options.exit_on_keydown},
     "exit on key down",
     ""},
    {"exitonmousedown",
     OPT_BOOL | OPT_EXPERT,
     {&g_player_options.exit_on_mousedown},
     "exit on mouse down",
     ""},
    {"framedrop",
     OPT_BOOL | OPT_EXPERT,
     {&g_options.framedrop},
     "drop frames when cpu is too slow",
     ""},
    {"infbuf",
     OPT_BOOL | OPT_EXPERT,
     {&g_options.infinite_buffer},
     "don't limit the input buffer size (useful with realtime streams)",
     ""},
#if CONFIG_AVFILTER
    {"vf",
     OPT_EXPERT | HAS_ARG,
     {.func_arg = opt_add_vfilter},
     "set video filters",
     "filter_graph"},
    {"af", OPT_STRING | HAS_ARG, {&g_options.afilters}, "set audio filters", "filter_graph"},
#endif
    {"default",
     HAS_ARG | OPT_AUDIO | OPT_VIDEO | OPT_EXPERT,
     {.func_arg = opt_default},
     "generic catch all option",
     ""},
    {"codec", HAS_ARG, {.func_arg = opt_codec}, "force decoder", "decoder_name"},
    {"acodec",
     HAS_ARG | OPT_STRING | OPT_EXPERT,
     {&g_options.audio_codec_name},
     "force audio decoder",
     "decoder_name"},
    {"vcodec",
     HAS_ARG | OPT_STRING | OPT_EXPERT,
     {&g_options.video_codec_name},
     "force video decoder",
     "decoder_name"},
    {"autorotate", OPT_BOOL, {&g_options.autorotate}, "automatically rotate video", ""},
    {
        NULL,
    }};

void show_usage(void) {
  INFO_LOG() << "Simple media player\nusage: " PROJECT_NAME_TITLE " [options] input_file";
}
}

void show_help_default(const char* opt, const char* arg) {
  UNUSED(opt);
  UNUSED(arg);

  show_usage();
  show_help_options(options, "Main options:", 0, OPT_EXPERT, 0);
  show_help_options(options, "Advanced options:", OPT_EXPERT, 0, 0);
  printf("\n");
  show_help_children(avcodec_get_class(), AV_OPT_FLAG_DECODING_PARAM);
  show_help_children(avformat_get_class(), AV_OPT_FLAG_DECODING_PARAM);
#if !CONFIG_AVFILTER
  show_help_children(sws_get_class(), AV_OPT_FLAG_ENCODING_PARAM);
#else
  show_help_children(avfilter_get_class(), AV_OPT_FLAG_FILTERING_PARAM);
#endif
  printf(
      "\nWhile playing:\n"
      "q, ESC              quit\n"
      "f                   toggle full screen\n"
      "p, SPC              pause\n"
      "m                   toggle mute\n"
      "9, 0                decrease and increase volume respectively\n"
      "/, *                decrease and increase volume respectively\n"
      "a                   cycle audio channel in the current program\n"
      "v                   cycle video channel\n"
      "c                   cycle program\n"
      "w                   cycle video filters or show modes\n"
      "s                   activate frame-step mode\n"
      "left/right          seek backward/forward 10 seconds\n"
      "down/up             seek backward/forward 1 minute\n"
      "page down/page up   seek backward/forward 10 minutes\n"
      "right mouse click   seek to percentage in file corresponding to fraction of width\n"
      "left double-click   toggle full screen\n");
}

static DictionaryOptions* dict = NULL;  // FIXME

template <typename B>
class FFmpegApplication : public B {
 public:
  typedef B base_class_t;
  FFmpegApplication(int argc, char** argv) : base_class_t(argc, argv), dict_(NULL) {
    init_dynload();

    parse_loglevel(argc, argv, options);

/* register all codecs, demux and protocols */
#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
#if CONFIG_AVFILTER
    avfilter_register_all();
#endif
    av_register_all();
    avformat_network_init();

    DictionaryOptions* lopt = new DictionaryOptions;

    signal(SIGINT, sigterm_handler);  /* Interrupt (ANSI).    */
    signal(SIGTERM, sigterm_handler); /* Termination (ANSI).  */

    show_banner(argc, argv, options);
    parse_options(argc, argv, options, lopt);
    dict = dict_ = lopt;
  }

  virtual int PreExec() override {
    g_options.autorotate = true;  // fix me
    if (av_lockmgr_register(lockmgr)) {
      ERROR_LOG() << "Could not initialize lock manager!";
      return EXIT_FAILURE;
    }

    int pre_ret = base_class_t::PreExec();
    fasto::fastotv::core::events::PreExecInfo inf(pre_ret);
    fasto::fastotv::core::events::PreExecEvent* pre_event =
        new fasto::fastotv::core::events::PreExecEvent(this, inf);
    base_class_t::SendEvent(pre_event);
    return pre_ret;
  }

  virtual int PostExec() override {
    int post_ret = base_class_t::PostExec();
    fasto::fastotv::core::events::PostExecInfo inf(post_ret);
    fasto::fastotv::core::events::PostExecEvent* post_event =
        new fasto::fastotv::core::events::PostExecEvent(this, inf);
    base_class_t::SendEvent(post_event);
    return post_ret;
  }

  ~FFmpegApplication() {
    av_lockmgr_register(NULL);
    destroy(&dict_);
    avformat_network_deinit();
    if (g_options.show_status) {
      printf("\n");
    }
  }

  DictionaryOptions* GetDict() const { return dict_; }

 private:
  static int lockmgr(void** mtx, enum AVLockOp op) {
    common::mutex* lmtx = static_cast<common::mutex*>(*mtx);
    switch (op) {
      case AV_LOCK_CREATE: {
        *mtx = new common::mutex;
        if (!*mtx) {
          return 1;
        }
        return 0;
      }
      case AV_LOCK_OBTAIN: {
        lmtx->lock();
        return 0;
      }
      case AV_LOCK_RELEASE: {
        lmtx->unlock();
        return 0;
      }
      case AV_LOCK_DESTROY: {
        delete lmtx;
        return 0;
      }
    }
    return 1;
  }

  DictionaryOptions* dict_;
};

common::application::IApplicationImpl* CreateApplicationImpl(int argc, char** argv) {
  return new FFmpegApplication<fasto::fastotv::core::application::Sdl2Application>(argc, argv);
}

/* Called from the main */
int main(int argc, char** argv) {
#if defined(NDEBUG)
  common::logging::LEVEL_LOG level = common::logging::L_INFO;
#else
  common::logging::LEVEL_LOG level = common::logging::L_INFO;
#endif
#if defined(LOG_TO_FILE)
  std::string log_path = common::file_system::prepare_path("~/" PROJECT_NAME_LOWERCASE ".log");
  INIT_LOGGER(PROJECT_NAME_TITLE, log_path, level);
#else
  INIT_LOGGER(PROJECT_NAME_TITLE, level);
#endif
  common::application::Application app(argc, argv, &CreateApplicationImpl);
  fasto::fastotv::core::ComplexOptions copt(dict->swr_opts, dict->sws_dict, dict->format_opts,
                                            dict->codec_opts);
  fasto::fastotv::Player* player = new fasto::fastotv::Player(g_player_options, g_options, copt);
  int res = app.Exec();
  destroy(&player);
  return res;
}
