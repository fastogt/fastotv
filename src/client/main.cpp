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

#include "inih/ini.h"

#define CONFIG_MAIN_OPTIONS "main_options"
#define CONFIG_MAIN_OPTIONS_LOG_LEVEL_FIELD "loglevel"
#define CONFIG_MAIN_OPTIONS_POWEROFF_ON_EXIT_FIELD "poweroffonexit"

#define CONFIG_PLAYER_OPTIONS "player_options"
#define CONFIG_PLAYER_OPTIONS_WIDTH_FIELD "width"
#define CONFIG_PLAYER_OPTIONS_HEIGHT_FIELD "height"
#define CONFIG_PLAYER_OPTIONS_FULLSCREEN_FIELD "fullscreen"
#define CONFIG_PLAYER_OPTIONS_VOLUME_FIELD "volume"
#define CONFIG_PLAYER_OPTIONS_EXIT_ON_KEYDOWN_FIELD "exitonkeydown"
#define CONFIG_PLAYER_OPTIONS_EXIT_ON_MOUSEDOWN_FIELD "exitonmousedown"

#define CONFIG_APP_OPTIONS "app_options"
#define CONFIG_APP_OPTIONS_AST_FIELD "ast"
#define CONFIG_APP_OPTIONS_VST_FIELD "vst"
#define CONFIG_APP_OPTIONS_STATS_FIELD "stats"
#define CONFIG_APP_OPTIONS_FAST_FIELD "fast"
#define CONFIG_APP_OPTIONS_GENPTS_FIELD "genpts"
#define CONFIG_APP_OPTIONS_LOWRES_FIELD "lowres"
#define CONFIG_APP_OPTIONS_SYNC_FIELD "sync"
#define CONFIG_APP_OPTIONS_FRAMEDROP_FIELD "framedrop"
#define CONFIG_APP_OPTIONS_INFBUF_FIELD "infbuf"
#define CONFIG_APP_OPTIONS_VF_FIELD "vf"
#define CONFIG_APP_OPTIONS_AF_FIELD "af"
#define CONFIG_APP_OPTIONS_ACODEC_FIELD "acodec"
#define CONFIG_APP_OPTIONS_VCODEC_FIELD "vcodec"
#define CONFIG_APP_OPTIONS_HWACCEL_FIELD "hwaccel"
#define CONFIG_APP_OPTIONS_HWACCEL_DEVICE_FIELD "hwaccel_device"
#define CONFIG_APP_OPTIONS_HWACCEL_OUTPUT_FORMAT_FIELD "hwaccel_output_format"
#define CONFIG_APP_OPTIONS_AUTOROTATE_FIELD "autorotate"

#define CONFIG_OTHER_OPTIONS "other"
#define CONFIG_OTHER_OPTIONS_VIDEO_SIZE_FIELD "video_size"
#define CONFIG_OTHER_OPTIONS_PIXEL_FORMAT_FIELD "pixel_format"

#undef ERROR

// vaapi args: -hwaccel vaapi -hwaccel_device /dev/dri/card0
// vdpau args: -hwaccel vdpau
// scale output: -vf scale=1920x1080

/*
  [main_options]
  loglevel=INFO ["EMERG", "ALLERT", "CRITICAL", "ERROR", "WARNING", "NOTICE", "INFO", "DEBUG"]
  poweroffonexit=false [true,false]

  [app_options]
  ast=0 [0, INT_MAX]
  vst=0 [0, INT_MAX]
  stats=true [true,false]
  fast=false [true,false]
  genpts=false [true,false]
  lowres=0 [0, INT_MAX]
  sync=audio [audio, video]
  framedrop=-1 [-1, 0, 1]
  infbuf=-1 [-1, 0, 1]
  vf=std::string() []
  af=std::string() []
  acodec=std::string() []
  vcodec=std::string() []
  hwaccel=none [none, auto, vdpau, dxva2, vda,
               videotoolbox, qsv, vaapi, cuvid]
  hwaccel_device=std::string() []
  hwaccel_output_format=std::string() []
  autorotate=false [true,false]

  [player_options]
  width=0  [0, INT_MAX]
  height=0 [0, INT_MAX]
  fullscreen=false [true,false]
  volume=100 [0,100]
  exitonkeydown=false [true,false]
  exitonmousedown=false [true,false]

  [other]
  video_size=0 [0, INT_MAX]
  pixel_format=std::string() []
*/

namespace {
struct MainOptions {
  MainOptions()
      : power_off_on_exit(false),
        loglevel(common::logging::L_INFO),
        app_options(),
        player_options(),
        dict(new DictionaryOptions) {}
  ~MainOptions() { destroy(&dict); }

  bool power_off_on_exit;
  common::logging::LEVEL_LOG loglevel;

  fasto::fastotv::client::core::AppOptions app_options;
  fasto::fastotv::client::PlayerOptions player_options;
  DictionaryOptions* dict;

 private:
  DISALLOW_COPY_AND_ASSIGN(MainOptions);
};

void sigterm_handler(int sig) {
  UNUSED(sig);
  exit(EXIT_FAILURE);
}

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
  FFmpegApplication(int argc, char** argv) : base_class_t(argc, argv) {
    init_dynload();

/* register all codecs, demux and protocols */
#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
#if CONFIG_AVFILTER
    avfilter_register_all();
#endif
    av_register_all();
    avformat_network_init();

    signal(SIGINT, sigterm_handler);  /* Interrupt (ANSI).    */
    signal(SIGTERM, sigterm_handler); /* Termination (ANSI).  */

    av_log_set_callback(avlog_cb);
    int ffmpeg_log_level = fasto_log_to_ffmpeg(common::logging::CURRENT_LOG_LEVEL());
    av_log_set_level(ffmpeg_log_level);
  }

  virtual int PreExec() override {
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
    avformat_network_deinit();
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
      std::cout << "Can't create app directory error:(" << err->Description()
                << "), path: " << app_directory_absolute_path << std::endl;
      return EXIT_FAILURE;
    }
  }

  common::ErrnoError err = common::file_system::node_access(app_directory_absolute_path);
  if (err && err->IsError()) {
    std::cout << "Can't have permissions to app directory path: " << app_directory_absolute_path
              << std::endl;
    return EXIT_FAILURE;
  }

  if (!common::file_system::is_directory_exist(runtime_directory_absolute_path)) {
    common::ErrnoError err =
        common::file_system::create_directory(runtime_directory_absolute_path, true);
    if (err && err->IsError()) {
      std::cout << "Can't create runtime directory error:(" << err->Description()
                << "), path: " << runtime_directory_absolute_path << std::endl;
      return EXIT_FAILURE;
    }
  }

  err = common::file_system::node_access(runtime_directory_absolute_path);
  if (err && err->IsError()) {
    std::cout << "Can't have permissions to runtime directory path: "
              << runtime_directory_absolute_path << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

static int ini_handler_fasto(void* user, const char* section, const char* name, const char* value) {
  MainOptions* pconfig = reinterpret_cast<MainOptions*>(user);
  size_t value_len = strlen(value);
  if (value_len == 0) {  // skip empty fields
    return 0;
  }

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
  if (MATCH(CONFIG_MAIN_OPTIONS, CONFIG_MAIN_OPTIONS_LOG_LEVEL_FIELD)) {
    common::logging::LEVEL_LOG lg;
    if (common::logging::text_to_log_level(value, &lg)) {
      pconfig->loglevel = lg;
    }
    return 1;
  } else if (MATCH(CONFIG_MAIN_OPTIONS, CONFIG_MAIN_OPTIONS_POWEROFF_ON_EXIT_FIELD)) {
    bool exit;
    if (parse_bool(value, &exit)) {
      pconfig->power_off_on_exit = exit;
    }
    return 1;
  } else if (MATCH(CONFIG_PLAYER_OPTIONS, CONFIG_PLAYER_OPTIONS_WIDTH_FIELD)) {
    int width;
    if (parse_number(value, 0, std::numeric_limits<int>::max(), &width)) {
      pconfig->player_options.screen_size.width = width;
    }
    return 1;
  } else if (MATCH(CONFIG_PLAYER_OPTIONS, CONFIG_PLAYER_OPTIONS_HEIGHT_FIELD)) {
    int height;
    if (parse_number(value, 0, std::numeric_limits<int>::max(), &height)) {
      pconfig->player_options.screen_size.height = height;
    }
    return 1;
  } else if (MATCH(CONFIG_PLAYER_OPTIONS, CONFIG_PLAYER_OPTIONS_FULLSCREEN_FIELD)) {
    bool is_full_screen;
    if (parse_bool(value, &is_full_screen)) {
      pconfig->player_options.is_full_screen = is_full_screen;
    }
    return 1;
  } else if (MATCH(CONFIG_PLAYER_OPTIONS, CONFIG_PLAYER_OPTIONS_VOLUME_FIELD)) {
    int volume;
    if (parse_number(value, 0, 100, &volume)) {
      pconfig->player_options.audio_volume = volume;
    }
    return 1;
  } else if (MATCH(CONFIG_PLAYER_OPTIONS, CONFIG_PLAYER_OPTIONS_EXIT_ON_KEYDOWN_FIELD)) {
    bool exit;
    if (parse_bool(value, &exit)) {
      pconfig->player_options.exit_on_keydown = exit;
    }
    return 1;
  } else if (MATCH(CONFIG_PLAYER_OPTIONS, CONFIG_PLAYER_OPTIONS_EXIT_ON_MOUSEDOWN_FIELD)) {
    bool exit;
    if (parse_bool(value, &exit)) {
      pconfig->player_options.exit_on_mousedown = exit;
    }
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_AST_FIELD)) {
    pconfig->app_options.wanted_stream_spec[AVMEDIA_TYPE_AUDIO] = value;
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_VST_FIELD)) {
    pconfig->app_options.wanted_stream_spec[AVMEDIA_TYPE_VIDEO] = value;
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_STATS_FIELD)) {
    bool show_stats;
    if (parse_bool(value, &show_stats)) {
      pconfig->app_options.show_status = show_stats;
    }
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_FAST_FIELD)) {
    bool fast;
    if (parse_bool(value, &fast)) {
      pconfig->app_options.fast = fast;
    }
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_GENPTS_FIELD)) {
    bool genpts;
    if (parse_bool(value, &genpts)) {
      pconfig->app_options.genpts = genpts;
    }
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_LOWRES_FIELD)) {
    int lowres;
    if (parse_number(value, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(),
                     &lowres)) {
      pconfig->app_options.lowres = lowres;
    }
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_SYNC_FIELD)) {
    if (strcmp(value, "audio") == 0) {
      pconfig->app_options.av_sync_type = fasto::fastotv::client::core::AV_SYNC_AUDIO_MASTER;
    } else if (strcmp(value, "video") == 0) {
      pconfig->app_options.av_sync_type = fasto::fastotv::client::core::AV_SYNC_VIDEO_MASTER;
    } else {
      return 0;
    }
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_FRAMEDROP_FIELD)) {
    if (strcmp(value, "auto") == 0) {
      pconfig->app_options.framedrop = fasto::fastotv::client::core::FRAME_DROP_AUTO;
    } else if (strcmp(value, "off") == 0) {
      pconfig->app_options.framedrop = fasto::fastotv::client::core::FRAME_DROP_OFF;
    } else if (strcmp(value, "on") == 0) {
      pconfig->app_options.framedrop = fasto::fastotv::client::core::FRAME_DROP_ON;
    } else {
      return 0;
    }

    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_INFBUF_FIELD)) {
    int inf;
    if (parse_number(value, -1, 1, &inf)) {
      pconfig->app_options.infinite_buffer = inf;
    }
    return 1;
#if CONFIG_AVFILTER
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_VF_FIELD)) {
    const std::string arg_copy(value);
    size_t del = arg_copy.find_first_of('=');
    if (del != std::string::npos) {
      std::string key = arg_copy.substr(0, del);
      std::string value = arg_copy.substr(del + 1);
      fasto::fastotv::Size sz;
      if (key == "scale" && common::ConvertFromString(value, &sz)) {
        pconfig->player_options.screen_size = sz;
      }
    }
    pconfig->app_options.vfilters = value;
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_AF_FIELD)) {
    pconfig->app_options.afilters = value;
    return 1;
#endif
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_ACODEC_FIELD)) {
    pconfig->app_options.audio_codec_name = value;
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_VCODEC_FIELD)) {
    if (strcmp(value, "auto") == 0) {
      pconfig->app_options.hwaccel_id = fasto::fastotv::client::core::HWACCEL_AUTO;
    } else if (strcmp(value, "none") == 0) {
      pconfig->app_options.hwaccel_id = fasto::fastotv::client::core::HWACCEL_NONE;
    } else {
      for (size_t i = 0; i < fasto::fastotv::client::core::hwaccel_count(); i++) {
        if (strcmp(fasto::fastotv::client::core::hwaccels[i].name, value) == 0) {
          pconfig->app_options.hwaccel_id = fasto::fastotv::client::core::hwaccels[i].id;
          return 1;
        }
      }
      return 0;
    }
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_HWACCEL_FIELD)) {
    pconfig->app_options.video_codec_name = value;
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_HWACCEL_DEVICE_FIELD)) {
    pconfig->app_options.hwaccel_device = value;
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_HWACCEL_OUTPUT_FORMAT_FIELD)) {
    pconfig->app_options.hwaccel_output_format = value;
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_AUTOROTATE_FIELD)) {
    bool autorotate;
    if (parse_bool(value, &autorotate)) {
      pconfig->app_options.autorotate = autorotate;
    }
    return 1;
  } else if (MATCH(CONFIG_OTHER_OPTIONS, CONFIG_OTHER_OPTIONS_VIDEO_SIZE_FIELD)) {
    opt_default("video_size", value, pconfig->dict);
    return 1;
  } else if (MATCH(CONFIG_OTHER_OPTIONS, CONFIG_OTHER_OPTIONS_PIXEL_FORMAT_FIELD)) {
    opt_default("pixel_format", value, pconfig->dict);
    return 1;
  } else {
    return 0; /* unknown section/name, error */
  }
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

  const std::string config_absolute_path =
      common::file_system::make_path(app_directory_absolute_path, CONFIG_FILE_NAME);
  if (!common::file_system::is_valid_path(config_absolute_path)) {
    std::cout << "Can't get pid file path: " << config_absolute_path << std::endl;
    return EXIT_FAILURE;
  }

  MainOptions main_options;
  const char* config_absolute_path_ptr = common::utils::c_strornull(config_absolute_path);
  if (ini_parse(config_absolute_path_ptr, ini_handler_fasto, &main_options) < 0) {
    std::cout << "Can't load config " << config_absolute_path << ", use default settings."
              << std::endl;
  }

#if defined(LOG_TO_FILE)
  const std::string log_path =
      common::file_system::make_path(app_directory_absolute_path, std::string(LOG_FILE_NAME));
  INIT_LOGGER(PROJECT_NAME_TITLE, log_path, main_options.loglevel);
#else
  INIT_LOGGER(PROJECT_NAME_TITLE, main_options.loglevel);
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

  DictionaryOptions* dict = main_options.dict;
  fasto::fastotv::client::core::ComplexOptions copt(dict->swr_opts, dict->sws_dict,
                                                    dict->format_opts, dict->codec_opts);
  fasto::fastotv::client::Player* player = new fasto::fastotv::client::Player(
      main_options.player_options, main_options.app_options, copt);
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

  if (main_options.power_off_on_exit) {
    common::Error err_shut = common::system::Shutdown(common::system::SHUTDOWN);
    if (err_shut && err_shut->IsError()) {
      WARNING_LOG() << "Can't shutdown error: " << err_shut->Description();
    }
  }
  return res;
}

/* Called from the main */
int main(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
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
