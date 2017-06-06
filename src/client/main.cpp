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

#include <signal.h>  // for signal, SIGINT, SIGTERM
#include <stdarg.h>  // for va_list
#include <stdint.h>  // for uint32_t
#include <stdlib.h>  // for EXIT_SUCCESS, EXIT_FAILURE
#include <string.h>  // for strcmp, NULL
#include <iostream>  // for ostream
#include <string>    // for string

#include "ffmpeg_config.h"  // for CONFIG_AVDEVICE, CONFIG_...

extern "C" {
#include <libavcodec/avcodec.h>    // for av_lockmgr_register, AVL...
#include <libavdevice/avdevice.h>  // for avdevice_register_all
#include <libavfilter/avfilter.h>  // for avfilter_register_all
#include <libavformat/avformat.h>  // for av_register_all, avforma...
#include <libavutil/log.h>         // for AV_LOG_ERROR, AV_LOG_FATAL
}

#include <common/application/application.h>  // for Application, IApplicatio...
#include <common/error.h>                    // for ErrnoError, Error
#include <common/file_system.h>              // for File, create_directory
#include <common/log_levels.h>               // for LEVEL_LOG, LEVEL_LOG::L_...
#include <common/logger.h>                   // for COMPACT_LOG_ERROR, ERROR...
#include <common/macros.h>                   // for ERROR_RESULT_VALUE, UNUSED
#include <common/string_util.h>
#include <common/system/system.h>            // for Shutdown, shutdown_t::SH...
#include <common/threads/types.h>            // for mutex
#include <common/utils.h>

#include "client/cmdutils.h"            // for DictionaryOptions, show_...
#include "client/core/app_options.h"    // for ComplexOptions
#include "client/core/events/events.h"  // for PostExecEvent, PostExecInfo
#include "client/player.h"              // for Player
#include "client/core/application/sdl2_application.h"

#include "client/config.h"                     // for TVConfig, load_config_file

#undef ERROR

namespace {

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
  int res = common::vasprintf(&ret, sz_fmt, varg);
  if (res == ERROR_RESULT_VALUE || !ret) {
    return;
  }

  static std::ostream& info_stream = common::logging::LogMessage(common::logging::L_INFO, false).Stream();
  info_stream << ret;
  free(ret);
}
}  // namespace

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
  return new FFmpegApplication<fasto::fastotv::client::core::application::Sdl2Application>(argc, argv);
}

static int prepare_to_start(const std::string& app_directory_absolute_path,
                            const std::string& runtime_directory_absolute_path) {
  if (!common::file_system::is_directory_exist(app_directory_absolute_path)) {
    common::ErrnoError err = common::file_system::create_directory(app_directory_absolute_path, true);
    if (err && err->IsError()) {
      std::cout << "Can't create app directory error:(" << err->Description()
                << "), path: " << app_directory_absolute_path << std::endl;
      return EXIT_FAILURE;
    }
  }

  common::ErrnoError err = common::file_system::node_access(app_directory_absolute_path);
  if (err && err->IsError()) {
    std::cout << "Can't have permissions to app directory path: " << app_directory_absolute_path << std::endl;
    return EXIT_FAILURE;
  }

  if (!common::file_system::is_directory_exist(runtime_directory_absolute_path)) {
    common::ErrnoError err = common::file_system::create_directory(runtime_directory_absolute_path, true);
    if (err && err->IsError()) {
      std::cout << "Can't create runtime directory error:(" << err->Description()
                << "), path: " << runtime_directory_absolute_path << std::endl;
      return EXIT_FAILURE;
    }
  }

  err = common::file_system::node_access(runtime_directory_absolute_path);
  if (err && err->IsError()) {
    std::cout << "Can't have permissions to runtime directory path: " << runtime_directory_absolute_path << std::endl;
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

  fasto::fastotv::client::TVConfig main_options;
  const std::string config_absolute_path =
      common::file_system::make_path(app_directory_absolute_path, CONFIG_FILE_NAME);
  if (!common::file_system::is_valid_path(config_absolute_path)) {
    std::cout << "Can't config file path: " << config_absolute_path << std::endl;
    return EXIT_FAILURE;
  }

  common::Error err = load_config_file(config_absolute_path, &main_options);
  if (err && err->IsError()) {
    return EXIT_FAILURE;
  }

#if defined(LOG_TO_FILE)
  const std::string log_path = common::file_system::make_path(app_directory_absolute_path, std::string(LOG_FILE_NAME));
  INIT_LOGGER(PROJECT_NAME_TITLE, log_path, main_options.loglevel);
#else
  INIT_LOGGER(PROJECT_NAME_TITLE, main_options.loglevel);
#endif

  common::application::Application app(argc, argv, &CreateApplicationImpl);

  const std::string pid_absolute_path = common::file_system::make_path(runtime_directory_absolute_path, PID_FILE_NAME);
  if (!common::file_system::is_valid_path(pid_absolute_path)) {
    ERROR_LOG() << "Can't get pid file path: " << pid_absolute_path;
    return EXIT_FAILURE;
  }

  const uint32_t fl = common::file_system::File::FLAG_CREATE | common::file_system::File::FLAG_WRITE;
  common::file_system::File lock_pid_file;
  err = lock_pid_file.Open(pid_absolute_path, fl);
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
  fasto::fastotv::client::core::ComplexOptions copt(dict->swr_opts, dict->sws_dict, dict->format_opts,
                                                    dict->codec_opts);
  fasto::fastotv::client::Player* player =
      new fasto::fastotv::client::Player(main_options.player_options, main_options.app_options, copt);
  res = app.Exec();
  destroy(&player);

  err = lock_pid_file.Unlock();
  if (err && err->IsError()) {
    WARNING_LOG() << "Can't unlock pid file path: " << pid_absolute_path;
  }

  err = lock_pid_file.Close();
  if (err && err->IsError()) {
    WARNING_LOG() << "Can't close pid file path: " << pid_absolute_path << ", error: " << err->Description();
  }

  err = common::file_system::remove_file(pid_absolute_path);
  if (err && err->IsError()) {
    WARNING_LOG() << "Can't remove file: " << pid_absolute_path << ", error: " << err->Description();
  }

  // save config file
  err = save_config_file(config_absolute_path, &main_options);
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
  return main_single_application(argc, argv, app_directory_absolute_path, runtime_directory_absolute_path);
}
