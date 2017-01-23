#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>

#include <common/macros.h>

extern "C" {
#include <SDL2/SDL.h>
#include "cmdutils.h"
}

#include "video_state.h"

core::AppOptions g_options;
AVInputFormat* file_iformat = NULL;

#if CONFIG_AVFILTER
static int opt_add_vfilter(void* optctx, const char* opt, const char* arg) {
  UNUSED(optctx);
  UNUSED(opt);

  g_options.initAvFilters(arg);
  return 0;
}
#endif

static void sigterm_handler(int sig) {
  UNUSED(sig);

  exit(123);
}

static int opt_frame_size(void* optctx, const char* opt, const char* arg) {
  UNUSED(optctx);
  UNUSED(opt);

  av_log(NULL, AV_LOG_WARNING, "Option -s is deprecated, use -video_size.\n");
  return opt_default(NULL, "video_size", arg);
}

static int opt_width(void* optctx, const char* opt, const char* arg) {
  UNUSED(optctx);

  g_options.screen_width = static_cast<int>(parse_number_or_die(opt, arg, OPT_INT64, 1, INT_MAX));
  return 0;
}

static int opt_height(void* optctx, const char* opt, const char* arg) {
  UNUSED(optctx);

  g_options.screen_height = static_cast<int>(parse_number_or_die(opt, arg, OPT_INT64, 1, INT_MAX));
  return 0;
}

static int opt_format(void* optctx, const char* opt, const char* arg) {
  UNUSED(optctx);
  UNUSED(opt);

  file_iformat = av_find_input_format(arg);
  if (!file_iformat) {
    av_log(NULL, AV_LOG_FATAL, "Unknown input format: %s\n", arg);
    return AVERROR(EINVAL);
  }
  return 0;
}

static int opt_frame_pix_fmt(void* optctx, const char* opt, const char* arg) {
  UNUSED(optctx);
  UNUSED(opt);

  av_log(NULL, AV_LOG_WARNING, "Option -pix_fmt is deprecated, use -pixel_format.\n");
  return opt_default(NULL, "pixel_format", arg);
}

static int opt_sync(void* optctx, const char* opt, const char* arg) {
  UNUSED(optctx);

  if (!strcmp(arg, "audio")) {
    g_options.av_sync_type = core::AV_SYNC_AUDIO_MASTER;
  } else if (!strcmp(arg, "video")) {
    g_options.av_sync_type = core::AV_SYNC_VIDEO_MASTER;
  } else if (!strcmp(arg, "ext")) {
    g_options.av_sync_type = core::AV_SYNC_EXTERNAL_CLOCK;
  } else {
    av_log(NULL, AV_LOG_ERROR, "Unknown value for %s: %s\n", opt, arg);
    exit(1);
  }
  return 0;
}

static int opt_seek(void* optctx, const char* opt, const char* arg) {
  UNUSED(optctx);

  g_options.start_time = parse_time_or_die(opt, arg, 1);
  return 0;
}

static int opt_duration(void* optctx, const char* opt, const char* arg) {
  UNUSED(optctx);

  g_options.duration = parse_time_or_die(opt, arg, 1);
  return 0;
}

static int opt_show_mode(void* optctx, const char* opt, const char* arg) {
  UNUSED(optctx);

  g_options.show_mode = !strcmp(arg, "video")
                            ? core::SHOW_MODE_VIDEO
                            : !strcmp(arg, "waves")
                                  ? core::SHOW_MODE_WAVES
                                  : !strcmp(arg, "rdft")
                                        ? core::SHOW_MODE_RDFT
                                        : static_cast<core::ShowMode>(parse_number_or_die(
                                              opt, arg, OPT_INT, 0, core::SHOW_MODE_NB - 1));
  return 0;
}

static int opt_input_file(void* optctx, const char* opt, const char* arg) {
  UNUSED(optctx);
  UNUSED(opt);

  if (!g_options.input_filename.empty()) {
    av_log(NULL, AV_LOG_FATAL,
           "Argument '%s' provided as input filename, but '%s' was already specified.\n", arg,
           g_options.input_filename.c_str());
    return AVERROR(EINVAL);
  }

  if (strcmp(arg, "-") == 0) {
    arg = "pipe:";
  }
  g_options.input_filename = arg;
  return 0;
}

static int opt_codec(void* optctx, const char* opt, const char* arg) {
  UNUSED(optctx);

  const char* spec = strchr(opt, ':');
  if (!spec) {
    av_log(NULL, AV_LOG_ERROR, "No media specifier was specified in '%s' in option '%s'\n", arg,
           opt);
    return AVERROR(EINVAL);
  }
  spec++;
  switch (spec[0]) {
    case 'a':
      g_options.audio_codec_name = arg;
      break;
    case 's':
      g_options.subtitle_codec_name = arg;
      break;
    case 'v':
      g_options.video_codec_name = arg;
      break;
    default:
      av_log(NULL, AV_LOG_ERROR, "Invalid media specifier '%s' in option '%s'\n", spec, opt);
      return AVERROR(EINVAL);
  }
  return 0;
}

static const OptionDef options[] = {
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
    {"report", 0, {(void*)opt_report}, "generate a report"},
    {"max_alloc",
     HAS_ARG,
     {.func_arg = opt_max_alloc},
     "set maximum size of a single allocated block",
     "bytes"},
    {"cpuflags",
     HAS_ARG | OPT_EXPERT,
     {.func_arg = opt_cpuflags},
     "force specific cpu flags",
     "flags"},
    {"hide_banner",
     OPT_BOOL | OPT_EXPERT,
     {&hide_banner},
     "do not show program banner",
     "hide_banner"},
#if CONFIG_OPENCL
    {"opencl_bench",
     OPT_EXIT,
     {.func_arg = opt_opencl_bench},
     "run benchmark on all OpenCL devices and show results"},
    {"opencl_options", HAS_ARG, {.func_arg = opt_opencl}, "set OpenCL environment options"},
#endif
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
    {"fs", OPT_BOOL, {&g_options.is_full_screen}, "force full screen"},
    {"an", OPT_BOOL, {&g_options.audio_disable}, "disable audio"},
    {"vn", OPT_BOOL, {&g_options.video_disable}, "disable video"},
    {"sn", OPT_BOOL, {&g_options.subtitle_disable}, "disable subtitling"},
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
    {"sst",
     OPT_STRING | HAS_ARG | OPT_EXPERT,
     {&g_options.wanted_stream_spec[AVMEDIA_TYPE_SUBTITLE]},
     "select desired subtitle stream",
     "stream_specifier"},
    {"ss", HAS_ARG, {.func_arg = opt_seek}, "seek to a given position in seconds", "pos"},
    {"t",
     HAS_ARG,
     {.func_arg = opt_duration},
     "play  \"duration\" seconds of audio/video",
     "duration"},
    {"bytes",
     OPT_INT | HAS_ARG,
     {&g_options.seek_by_bytes},
     "seek by bytes 0=off 1=on -1=auto",
     "val"},
    {"nodisp", OPT_BOOL, {&g_options.display_disable}, "disable graphical display"},
    {"volume",
     OPT_INT | HAS_ARG,
     {&g_options.startup_volume},
     "set startup volume 0=min 100=max",
     "volume"},
    {"f", HAS_ARG, {.func_arg = opt_format}, "force format", "fmt"},
    {"pix_fmt",
     HAS_ARG | OPT_EXPERT | OPT_VIDEO,
     {.func_arg = opt_frame_pix_fmt},
     "set pixel format",
     "format"},
    {"stats", OPT_BOOL | OPT_EXPERT, {&g_options.show_status}, "show status", ""},
    {"fast", OPT_BOOL | OPT_EXPERT, {&g_options.fast}, "non spec compliant optimizations", ""},
    {"genpts", OPT_BOOL | OPT_EXPERT, {&g_options.genpts}, "generate pts", ""},
    {"drp",
     OPT_INT | HAS_ARG | OPT_EXPERT,
     {&g_options.decoder_reorder_pts},
     "let decoder reorder pts 0=off 1=on -1=auto",
     ""},
    {"lowres", OPT_INT | HAS_ARG | OPT_EXPERT, {&g_options.lowres}, "", ""},
    {"sync",
     HAS_ARG | OPT_EXPERT,
     {.func_arg = opt_sync},
     "set audio-video sync. type (type=audio/video/ext)",
     "type"},
    {"autoexit", OPT_BOOL | OPT_EXPERT, {&g_options.autoexit}, "exit at the end", ""},
    {"exitonkeydown", OPT_BOOL | OPT_EXPERT, {&g_options.exit_on_keydown}, "exit on key down", ""},
    {"exitonmousedown",
     OPT_BOOL | OPT_EXPERT,
     {&g_options.exit_on_mousedown},
     "exit on mouse down",
     ""},
    {"loop",
     OPT_INT | HAS_ARG | OPT_EXPERT,
     {&g_options.loop},
     "set number of times the playback shall be looped",
     "loop count"},
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
    {"window_title",
     OPT_STRING | HAS_ARG,
     {&g_options.window_title},
     "set window title",
     "window title"},
#if CONFIG_AVFILTER
    {"vf",
     OPT_EXPERT | HAS_ARG,
     {.func_arg = opt_add_vfilter},
     "set video filters",
     "filter_graph"},
    {"af", OPT_STRING | HAS_ARG, {&g_options.afilters}, "set audio filters", "filter_graph"},
#endif
    {"rdftspeed",
     OPT_INT | HAS_ARG | OPT_AUDIO | OPT_EXPERT,
     {&g_options.rdftspeed},
     "rdft speed",
     "msecs"},
    {"showmode",
     HAS_ARG,
     {.func_arg = opt_show_mode},
     "select show mode (0 = video, 1 = waves, 2 = RDFT)",
     "mode"},
    {"default",
     HAS_ARG | OPT_AUDIO | OPT_VIDEO | OPT_EXPERT,
     {.func_arg = opt_default},
     "generic catch all option",
     ""},
    {"i", HAS_ARG, {.func_arg = opt_input_file}, "read specified file", "input_file"},
    {"codec", HAS_ARG, {.func_arg = opt_codec}, "force decoder", "decoder_name"},
    {"acodec",
     HAS_ARG | OPT_STRING | OPT_EXPERT,
     {&g_options.audio_codec_name},
     "force audio decoder",
     "decoder_name"},
    {"scodec",
     HAS_ARG | OPT_STRING | OPT_EXPERT,
     {&g_options.subtitle_codec_name},
     "force subtitle decoder",
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

static void show_usage(void) {
  av_log(NULL, AV_LOG_INFO, "Simple media player\n");
  av_log(NULL, AV_LOG_INFO, "usage: %s [options] input_file\n", PROJECT_NAME_TITLE);
  av_log(NULL, AV_LOG_INFO, "\n");
}

static int lockmgr(void** mtx, enum AVLockOp op) {
  SDL_mutex* lmtx = static_cast<SDL_mutex*>(*mtx);
  switch (op) {
    case AV_LOCK_CREATE: {
      *mtx = SDL_CreateMutex();
      if (!*mtx) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
        return 1;
      }
      return 0;
    }
    case AV_LOCK_OBTAIN: {
      return !!SDL_LockMutex(lmtx);
    }
    case AV_LOCK_RELEASE: {
      return !!SDL_UnlockMutex(lmtx);
    }
    case AV_LOCK_DESTROY: {
      SDL_DestroyMutex(lmtx);
      return 0;
    }
  }
  return 1;
}

static void do_init(int argc, char** argv) {
  init_dynload();

  av_log_set_flags(AV_LOG_SKIP_REPEATED);
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

  init_opts();

  signal(SIGINT, sigterm_handler);  /* Interrupt (ANSI).    */
  signal(SIGTERM, sigterm_handler); /* Termination (ANSI).  */

  show_banner(argc, argv, options);

  parse_options(NULL, argc, argv, options);
}

/* handle an event sent by the GUI */
static void do_exit() {
  av_lockmgr_register(NULL);
  uninit_opts();
  avformat_network_deinit();
  if (g_options.show_status) {
    printf("\n");
  }
  SDL_Quit();
  av_log(NULL, AV_LOG_QUIET, "%s", "");
}

void show_help_default(const char* opt, const char* arg) {
  UNUSED(opt);
  UNUSED(arg);

  av_log_set_callback(log_callback_help);
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
      "t                   cycle subtitle channel in the current program\n"
      "c                   cycle program\n"
      "w                   cycle video filters or show modes\n"
      "s                   activate frame-step mode\n"
      "left/right          seek backward/forward 10 seconds\n"
      "down/up             seek backward/forward 1 minute\n"
      "page down/page up   seek backward/forward 10 minutes\n"
      "right mouse click   seek to percentage in file corresponding to fraction of width\n"
      "left double-click   toggle full screen\n");
}

/* Called from the main */
int main(int argc, char** argv) {
  g_options.autorotate = 1;  // fix me
  do_init(argc, argv);

  if (g_options.input_filename.empty()) {
    show_usage();
    av_log(NULL, AV_LOG_FATAL, "An input file must be specified\n");
    av_log(NULL, AV_LOG_FATAL, "Use -h to get full help or, even better, run 'man %s'\n",
           PROJECT_NAME_TITLE);
    return EXIT_FAILURE;
  }

  if (g_options.display_disable) {
    g_options.video_disable = 1;
  }
  Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
  if (g_options.audio_disable) {
    flags &= ~SDL_INIT_AUDIO;
  } else {
    /* Try to work around an occasional ALSA buffer underflow issue when the
     * period size is NPOT due to ALSA resampling by forcing the buffer size. */
    if (!SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE")) {
      SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE", "1", 1);
    }
  }
  if (g_options.display_disable) {
    flags &= ~SDL_INIT_VIDEO;
  }
  if (SDL_Init(flags)) {
    av_log(NULL, AV_LOG_FATAL, "Could not initialize SDL - %s\n", SDL_GetError());
    av_log(NULL, AV_LOG_FATAL, "(Did you set the DISPLAY variable?)\n");
    return EXIT_FAILURE;
  }

  SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
  SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

  if (av_lockmgr_register(lockmgr)) {
    av_log(NULL, AV_LOG_FATAL, "Could not initialize lock manager!\n");
    do_exit();
    return EXIT_FAILURE;
  }

  core::ComplexOptions copt;
  copt.swr_opts = swr_opts;
  copt.sws_dict = sws_dict;
  copt.format_opts = format_opts;
  copt.codec_opts = codec_opts;
  VideoState* is = new VideoState(file_iformat, &g_options, &copt);
  int res = is->Exec();
  delete is;

  do_exit();
  return res;
}
