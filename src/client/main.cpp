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

#include <errno.h>   // for EINVAL
#include <signal.h>  // for signal, SIGINT, SIGTERM
#include <stdio.h>   // for printf
#include <stdlib.h>  // for exit, EXIT_FAILURE
#include <string.h>  // for strcmp, NULL, strchr
#include <limits>    // for numeric_limits
#include <ostream>   // for operator<<, basic_ostream
#include <string>    // for char_traits, string, etc
#include <iostream>

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
#include <common/utils.h>
#include <common/system/system.h>

#include "client/cmdutils.h"          // for HAS_ARG, OPT_EXPERT, etc
#include "client/core/app_options.h"  // for AppOptions, ComplexOptions
#include "client/core/types.h"
#include "client/core/application/sdl2_application.h"

#include "client/player.h"

#undef ERROR

// vaapi args: -hwaccel vaapi -hwaccel_device /dev/dri/card0
// vdpau args: -hwaccel vdpau
// scale output: -vf scale=1920:1080

namespace {
struct MainOptions {
  MainOptions() : power_off_on_exit(false) {}
  bool power_off_on_exit;
};
MainOptions g_main_options;
fasto::fastotv::client::core::AppOptions g_options;
fasto::fastotv::client::PlayerOptions g_player_options;
DictionaryOptions* dict = NULL;  // FIXME

#if CONFIG_AVFILTER
int opt_set_video_vfilter(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(dopt);

  if (!arg) {
    ERROR_LOG() << "Missing argument for option '" << opt << "'";
    return AVERROR(EINVAL);
  }

  const std::string arg_copy(arg);
  size_t del = arg_copy.find_first_of('=');
  if (del != std::string::npos) {
    std::string key = arg_copy.substr(0, del);
    std::string value = arg_copy.substr(del + 1);
    fasto::fastotv::Size sz;
    if (key == "scale" && common::ConvertFromString(value, &sz)) {
      g_player_options.screen_size = sz;
    }
  }
  g_options.vfilters = arg;
  return 0;
}

int opt_set_audio_vfilter(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(dopt);

  if (!arg) {
    ERROR_LOG() << "Missing argument for option '" << opt << "'";
    return AVERROR(EINVAL);
  }

  g_options.afilters = arg;
  return 0;
}
#endif

void sigterm_handler(int sig) {
  UNUSED(sig);
  exit(EXIT_FAILURE);
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

  if (!parse_number(arg, 1, std::numeric_limits<int>::max(), &g_player_options.screen_size.width)) {
    return ERROR_RESULT_VALUE;
  }
  return SUCCESS_RESULT_VALUE;
}

int opt_height(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);
  UNUSED(opt);

  if (!parse_number(arg, 1, std::numeric_limits<int>::max(),
                    &g_player_options.screen_size.height)) {
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
    g_options.av_sync_type = fasto::fastotv::client::core::AV_SYNC_AUDIO_MASTER;
  } else if (!strcmp(arg, "video")) {
    g_options.av_sync_type = fasto::fastotv::client::core::AV_SYNC_VIDEO_MASTER;
  } else {
    ERROR_LOG() << "Unknown value for " << opt << ": " << arg;
    exit(1);
  }
  return 0;
}

int opt_set_video_codec(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(dopt);

  if (!arg) {
    ERROR_LOG() << "Missing argument for option '" << opt << "'";
    return AVERROR(EINVAL);
  }

  g_options.video_codec_name = arg;
  return 0;
}

int opt_set_audio_codec(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(dopt);

  if (!arg) {
    ERROR_LOG() << "Missing argument for option '" << opt << "'";
    return AVERROR(EINVAL);
  }

  g_options.audio_codec_name = arg;
  return 0;
}

int opt_hwaccel(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(dopt);

  if (!strcmp(arg, "auto")) {
    g_options.hwaccel_id = fasto::fastotv::client::core::HWACCEL_AUTO;
  } else if (!strcmp(arg, "none")) {
    g_options.hwaccel_id = fasto::fastotv::client::core::HWACCEL_NONE;
  } else {
    for (size_t i = 0; i < fasto::fastotv::client::core::hwaccel_count(); i++) {
      if (!strcmp(fasto::fastotv::client::core::hwaccels[i].name, arg)) {
        g_options.hwaccel_id = fasto::fastotv::client::core::hwaccels[i].id;
        return 0;
      }
    }

    ERROR_LOG() << "Unknown value for " << opt << ": " << arg;
    exit(1);
  }
  return 0;
}

int opt_set_hw_device(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(dopt);

  if (!arg) {
    ERROR_LOG() << "Missing argument for option '" << opt << "'";
    return AVERROR(EINVAL);
  }

  g_options.hwaccel_device = arg;
  return 0;
}

int opt_set_hw_output_format(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(dopt);

  if (!arg) {
    ERROR_LOG() << "Missing argument for option '" << opt << "'";
    return AVERROR(EINVAL);
  }

  g_options.hwaccel_output_format = arg;
  return 0;
}

int opt_fullscreen(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(arg);
  UNUSED(dopt);

  g_player_options.is_full_screen = true;
  return 0;
}

int opt_select_audio_stream(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(dopt);

  if (!arg) {
    ERROR_LOG() << "Missing argument for option '" << opt << "'";
    return AVERROR(EINVAL);
  }

  g_options.wanted_stream_spec[AVMEDIA_TYPE_AUDIO] = arg;
  return 0;
}

int opt_select_video_stream(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(dopt);

  if (!arg) {
    ERROR_LOG() << "Missing argument for option '" << opt << "'";
    return AVERROR(EINVAL);
  }

  g_options.wanted_stream_spec[AVMEDIA_TYPE_VIDEO] = arg;
  return 0;
}

int opt_set_audio_volume(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(dopt);

  if (!arg) {
    ERROR_LOG() << "Missing argument for option '" << opt << "'";
    return AVERROR(EINVAL);
  }

  int vol;
  if (!parse_number(arg, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), &vol)) {
    return AVERROR(EINVAL);
  }
  g_player_options.audio_volume = vol;
  return 0;
}

int opt_set_show_status(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(arg);
  UNUSED(dopt);

  g_options.show_status = true;
  return 0;
}

int opt_set_non_spec(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(dopt);

  if (!arg) {
    ERROR_LOG() << "Missing argument for option '" << opt << "'";
    return AVERROR(EINVAL);
  }

  bool fast;
  if (!parse_bool(arg, &fast)) {
    return AVERROR(EINVAL);
  }
  g_options.fast = fast;
  return 0;
}

int opt_set_gen_pts(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(dopt);

  if (!arg) {
    ERROR_LOG() << "Missing argument for option '" << opt << "'";
    return AVERROR(EINVAL);
  }

  bool genpts;
  if (!parse_bool(arg, &genpts)) {
    return AVERROR(EINVAL);
  }
  g_options.genpts = genpts;
  return 0;
}

int opt_set_lowres_volume(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(dopt);

  if (!arg) {
    ERROR_LOG() << "Missing argument for option '" << opt << "'";
    return AVERROR(EINVAL);
  }

  int lowres;
  if (!parse_number(arg, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(),
                    &lowres)) {
    return AVERROR(EINVAL);
  }
  g_options.lowres = lowres;
  return 0;
}

int opt_set_exit_on_keydown(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(arg);
  UNUSED(dopt);

  g_player_options.exit_on_keydown = true;
  return 0;
}

int opt_set_power_off_on_exit(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(arg);
  UNUSED(dopt);

  g_main_options.power_off_on_exit = true;
  return 0;
}

int opt_set_exit_on_mousedown(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(arg);
  UNUSED(dopt);

  g_player_options.exit_on_mousedown = true;
  return 0;
}

int opt_set_frame_drop(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(arg);
  UNUSED(dopt);

  g_options.framedrop = fasto::fastotv::client::core::FRAME_DROP_ON;
  return 0;
}

int opt_set_infinite_buffer(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(arg);
  UNUSED(dopt);

  g_options.infinite_buffer = true;
  return 0;
}

int opt_set_autorotate(const char* opt, const char* arg, DictionaryOptions* dopt) {
  UNUSED(opt);
  UNUSED(arg);
  UNUSED(dopt);

  g_options.autorotate = true;
  return 0;
}

const OptionDef options[] = {
    {"loglevel", OPT_NOTHING, opt_loglevel, "set logging level", "loglevel"},
    {"x", OPT_NOTHING, opt_width, "force displayed width", "width"},
    {"y", OPT_NOTHING, opt_height, "force displayed height", "height"},
    {"s", OPT_VIDEO, opt_frame_size, "set frame size (WxH or abbreviation)", "size"},
    {"fs", OPT_NOTHING, opt_fullscreen, "force full screen", NULL},
    {"ast", OPT_EXPERT, opt_select_audio_stream, "select desired audio stream", "stream_specifier"},
    {"vst", OPT_EXPERT, opt_select_video_stream, "select desired video stream", "stream_specifier"},
    {"volume", OPT_NOTHING, opt_set_audio_volume, "set startup volume 0=min 100=max", "volume"},
    {"pix_fmt", OPT_EXPERT | OPT_VIDEO, opt_frame_pix_fmt, "set pixel format", "format"},
    {"stats", OPT_EXPERT, opt_set_show_status, "show status", ""},
    {"fast", OPT_EXPERT, opt_set_non_spec, "non spec compliant optimizations", ""},
    {"genpts", OPT_EXPERT, opt_set_gen_pts, "generate pts", ""},
    {"lowres", OPT_EXPERT, opt_set_lowres_volume, "", ""},
    {"sync", OPT_EXPERT, opt_sync, "set audio-video sync. type (type=audio/video)", "type"},
    {"exitonkeydown", OPT_EXPERT, opt_set_exit_on_keydown, "exit on key down", ""},
    {"exitonmousedown", OPT_EXPERT, opt_set_exit_on_mousedown, "exit on mouse down", ""},
    {"poweroffonexit", OPT_EXPERT, opt_set_power_off_on_exit, "poweroff on exit application", ""},
    {"framedrop", OPT_EXPERT, opt_set_frame_drop, "drop frames when cpu is too slow", ""},
    {"infbuf", OPT_EXPERT, opt_set_infinite_buffer,
     "don't limit the input buffer size (useful with realtime streams)", ""},
#if CONFIG_AVFILTER
    {"vf", OPT_EXPERT, opt_set_video_vfilter, "set video filters", "filter_graph"},
    {"af", OPT_NOTHING, opt_set_audio_vfilter, "set audio filters", "filter_graph"},
#endif
    {"default", OPT_AUDIO | OPT_VIDEO | OPT_EXPERT, opt_default, "generic catch all option", ""},
    {"acodec", OPT_EXPERT, opt_set_audio_codec, "force audio decoder", "decoder_name"},
    {"vcodec", OPT_EXPERT, opt_set_video_codec, "force video decoder", "decoder_name"},
    {"hwaccel", OPT_EXPERT, opt_hwaccel, "use HW accelerated decoding", "hwaccel name"},
    {"hwaccel_device", OPT_VIDEO | OPT_EXPERT | OPT_INPUT, opt_set_hw_device,
     "select a device for HW acceleration", "devicename"},
    {"hwaccel_output_format", OPT_VIDEO | OPT_EXPERT | OPT_INPUT, opt_set_hw_output_format,
     "select output format used with HW accelerated decoding", "format"},
    {"autorotate", OPT_NOTHING, opt_set_autorotate, "automatically rotate video", ""},
    {NULL, OPT_NOTHING, NULL, NULL, NULL}};

int fasto_log_to_ffmpeg(common::logging::LEVEL_LOG level) {
  if (level <= common::logging::L_CRIT) {
    return AV_LOG_FATAL;
  } else if (level <= common::logging::L_ERROR) {
    return AV_LOG_ERROR;
  } else if (level <= common::logging::L_WARNING) {
    return AV_LOG_WARNING;
  } else if (level <= common::logging::L_INFO) {
    return AV_LOG_INFO;
  } else {
    return common::logging::L_DEBUG;
  }
}

common::logging::LEVEL_LOG ffmpeg_log_to_fasto(int level) {
  if (level <= AV_LOG_FATAL) {
    return common::logging::L_CRIT;
  } else if (level <= AV_LOG_ERROR) {
    return common::logging::L_ERROR;
  } else if (level <= AV_LOG_WARNING) {
    return common::logging::L_WARNING;
  } else if (level <= AV_LOG_INFO) {
    return common::logging::L_INFO;
  } else {
    return common::logging::L_DEBUG;
  }
}

void avlog_cb(void*, int level, const char* sz_fmt, va_list varg) {
  common::logging::LEVEL_LOG lg = ffmpeg_log_to_fasto(level);
  common::logging::LEVEL_LOG clg = common::logging::CURRENT_LOG_LEVEL();
  if (lg > clg) {
    return;
  }

  char* ret = NULL;
#ifdef OS_POSIX
  int res = vasprintf(&ret, sz_fmt, varg);
#else
  int res = ERROR_RESULT_VALUE;
#endif
  if (res == ERROR_RESULT_VALUE) {
    return;
  }

  static std::ostream& info_stream =
      common::logging::LogMessage(common::logging::L_INFO, false).Stream();
  info_stream << ret;
  free(ret);
}
}

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
    av_log_set_callback(avlog_cb);
    int ffmpeg_log_level = fasto_log_to_ffmpeg(common::logging::CURRENT_LOG_LEVEL());
    av_log_set_level(ffmpeg_log_level);
  }

  virtual int PreExec() override {
    g_options.autorotate = true;  // fix me
    if (av_lockmgr_register(lockmgr)) {
      ERROR_LOG() << "Could not initialize lock manager!";
      return EXIT_FAILURE;
    }

    int pre_exec = base_class_t::PreExec();
    fasto::fastotv::client::core::events::PreExecInfo inf(pre_exec);
    fasto::fastotv::client::core::events::PreExecEvent* pre_event =
        new fasto::fastotv::client::core::events::PreExecEvent(this, inf);
    base_class_t::SendEvent(pre_event);
    return pre_exec;
  }

  virtual int PostExec() override {
    fasto::fastotv::client::core::events::PostExecInfo inf(EXIT_SUCCESS);
    fasto::fastotv::client::core::events::PostExecEvent* post_event =
        new fasto::fastotv::client::core::events::PostExecEvent(this, inf);
    base_class_t::SendEvent(post_event);
    return base_class_t::PostExec();
  }

  ~FFmpegApplication() {
    av_lockmgr_register(NULL);
    destroy(&dict_);
    avformat_network_deinit();
    if (g_options.show_status) {
      printf("\n");
    }
  }

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
  return new FFmpegApplication<fasto::fastotv::client::core::application::Sdl2Application>(argc,
                                                                                           argv);
}

static int prepare_to_start(const std::string& app_directory_absolute_path,
                            const std::string& runtime_directory_absolute_path) {
  if (!common::file_system::is_directory_exist(app_directory_absolute_path)) {
    common::ErrnoError err =
        common::file_system::create_directory(app_directory_absolute_path, true);
    if (err && err->IsError()) {
      ERROR_LOG() << "Can't create app directory error:(" << err->Description()
                  << "), path: " << app_directory_absolute_path;
      return EXIT_FAILURE;
    }
  }

  common::ErrnoError err = common::file_system::node_access(app_directory_absolute_path);
  if (err && err->IsError()) {
    ERROR_LOG() << "Can't have permissions to app directory path: " << app_directory_absolute_path;
    return EXIT_FAILURE;
  }

  if (!common::file_system::is_directory_exist(runtime_directory_absolute_path)) {
    common::ErrnoError err =
        common::file_system::create_directory(runtime_directory_absolute_path, true);
    if (err && err->IsError()) {
      ERROR_LOG() << "Can't create runtime directory error:(" << err->Description()
                  << "), path: " << runtime_directory_absolute_path;
      return EXIT_FAILURE;
    }
  }

  err = common::file_system::node_access(runtime_directory_absolute_path);
  if (err && err->IsError()) {
    ERROR_LOG() << "Can't have permissions to runtime directory path: "
                << runtime_directory_absolute_path;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

// runtime_directory_absolute_path can be not equal pwd (used for pid file location)
static int main_single_application(int argc,
                                   char** argv,
                                   const std::string& app_directory_absolute_path,
                                   const std::string& runtime_directory_absolute_path) {
  int res = prepare_to_start(app_directory_absolute_path, runtime_directory_absolute_path);
  if (res == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

#if defined(NDEBUG)
  common::logging::LEVEL_LOG level = common::logging::L_INFO;
#else
  common::logging::LEVEL_LOG level = common::logging::L_INFO;
#endif
#if defined(LOG_TO_FILE)
  const std::string log_path =
      common::file_system::make_path(app_directory_absolute_path, std::string(LOG_FILE_NAME));
  INIT_LOGGER(PROJECT_NAME_TITLE, log_path, level);
#else
  INIT_LOGGER(PROJECT_NAME_TITLE, level);
#endif

  common::application::Application app(argc, argv, &CreateApplicationImpl);

  const std::string pid_absolute_path =
      common::file_system::make_path(runtime_directory_absolute_path, PID_FILE_NAME);
  if (!common::file_system::is_valid_path(pid_absolute_path)) {
    ERROR_LOG() << "Can't get pid file path: " << pid_absolute_path;
    return EXIT_FAILURE;
  }

  const uint32_t fl =
      common::file_system::File::FLAG_CREATE | common::file_system::File::FLAG_WRITE;
  common::file_system::File lock_pid_file;
  common::ErrnoError err = lock_pid_file.Open(pid_absolute_path, fl);
  if (err && err->IsError()) {
    ERROR_LOG() << "Can't open pid file path: " << pid_absolute_path;
    return EXIT_FAILURE;
  }

  err = lock_pid_file.Lock();
  if (err && err->IsError()) {
    ERROR_LOG() << "Can't lock pid file path: " << pid_absolute_path;
    err = lock_pid_file.Close();
    return EXIT_FAILURE;
  }

  std::string pid_str = common::MemSPrintf("%ld\n", common::get_current_process_pid());
  size_t writed;
  err = lock_pid_file.Write(pid_str, &writed);
  if (err && err->IsError()) {
    ERROR_LOG() << "Can't write pid to file path: " << pid_absolute_path;
    err = lock_pid_file.Close();
    return EXIT_FAILURE;
  }

  fasto::fastotv::client::core::ComplexOptions copt(dict->swr_opts, dict->sws_dict,
                                                    dict->format_opts, dict->codec_opts);
  fasto::fastotv::client::Player* player =
      new fasto::fastotv::client::Player(g_player_options, g_options, copt);
  res = app.Exec();
  destroy(&player);

  err = lock_pid_file.Unlock();
  if (err && err->IsError()) {
    WARNING_LOG() << "Can't unlock pid file path: " << pid_absolute_path;
  }

  err = lock_pid_file.Close();
  err = common::file_system::remove_file(pid_absolute_path);
  if (err && err->IsError()) {
    WARNING_LOG() << "Can't remove file: " << pid_absolute_path
                  << ", error: " << err->Description();
  }

  if (g_main_options.power_off_on_exit) {
    common::Error err_shut = common::system::Shutdown(common::system::SHUTDOWN);
    if (err_shut && err_shut->IsError()) {
      WARNING_LOG() << "Can't shutdown error: " << err_shut->Description();
    }
  }
  return res;
}

/* Called from the main */
int main(int argc, char** argv) {
  for (int i = 0; i < argc; ++i) {
    const bool lastarg = i == argc - 1;
    if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
      show_version();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      std::string topic;
      if (!lastarg) {
        topic = argv[++i];
      }
      show_help(topic);
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--license") == 0 || strcmp(argv[i], "-l") == 0) {
      show_license();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--buildconf") == 0) {
      show_buildconf();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--formats") == 0) {
      show_formats();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--devices") == 0) {
      show_devices();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--codecs") == 0) {
      show_codecs();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--hwaccels") == 0) {
      show_hwaccels();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--decoders") == 0) {
      show_decoders();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--encoders") == 0) {
      show_encoders();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--bsfs") == 0) {
      show_bsfs();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--protocols") == 0) {
      show_protocols();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--filters") == 0) {
      show_filters();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--pix_fmts") == 0) {
      show_pix_fmts();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--layouts") == 0) {
      show_layouts();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--sample_fmts") == 0) {
      show_sample_fmts();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--colors") == 0) {
      show_colors();
      return EXIT_SUCCESS;
    }
#if CONFIG_AVDEVICE
    else if (strcmp(argv[i], "--sources") == 0) {
      std::string device;
      if (!lastarg) {
        device = argv[++i];
      }
      show_sources(device);
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "--sinks") == 0) {
      std::string device;
      if (!lastarg) {
        device = argv[++i];
      }
      show_sinks(device);
      return EXIT_SUCCESS;
    } else {
      show_help(std::string());
      return EXIT_SUCCESS;
    }
#endif
  }

  const std::string runtime_directory_path = RUNTIME_DIR;
  const std::string app_directory_path = APPLICATION_DIR;

  const std::string runtime_directory_absolute_path =
      common::file_system::is_absolute_path(runtime_directory_path)
          ? runtime_directory_path
          : common::file_system::absolute_path_from_relative(runtime_directory_path);

  const std::string app_directory_absolute_path =
      common::file_system::is_absolute_path(app_directory_path)
          ? app_directory_path
          : common::file_system::absolute_path_from_relative(app_directory_path);
  return main_single_application(argc, argv, app_directory_absolute_path,
                                 runtime_directory_absolute_path);
}
