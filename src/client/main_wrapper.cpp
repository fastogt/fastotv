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

#include "client/main_wrapper.h"

#include <signal.h>

#include <iostream>

extern "C" {
#include <libavformat/avformat.h>  // for av_register_all, avforma...
}

#include <common/error.h>
#include <common/file_system.h>
#include <common/sprintf.h>
#include <common/system/system.h>
#include <common/utils.h>

#include "client/config.h"
#include "client/player.h"  // for Player
#include "client/simple_player.h"

#include "client/core/application/sdl2_application.h"

namespace {

void __attribute__((noreturn)) sigterm_handler(int sig) {
  UNUSED(sig);
  exit(EXIT_FAILURE);
}

int fasto_log_to_ffmpeg(common::logging::LEVEL_LOG level) {
  if (level <= common::logging::L_CRIT) {
    return AV_LOG_FATAL;
  } else if (level <= common::logging::L_ERR) {
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
    return common::logging::L_ERR;
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

int prepare_to_start(const std::string& app_directory_absolute_path,
                     const std::string& runtime_directory_absolute_path) {
  if (!common::file_system::is_directory_exist(app_directory_absolute_path)) {
    common::ErrnoError err = common::file_system::create_directory(app_directory_absolute_path, true);
    if (err && err->IsError()) {
      std::cout << "Can't create app directory error:(" << err->GetDescription()
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
      std::cout << "Can't create runtime directory error:(" << err->GetDescription()
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

template <typename B>
class FFmpegApplication : public B {
 public:
  typedef B base_class_t;
  FFmpegApplication(int argc, char** argv) : base_class_t(argc, argv) {
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
    std::mutex* lmtx = static_cast<std::mutex*>(*mtx);
    switch (op) {
      case AV_LOCK_CREATE: {
        *mtx = new std::mutex;
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

}  // namespace

int main_simple_player_application(int argc,
                                   char** argv,
                                   const common::uri::Uri& stream_url,
                                   const std::string& app_directory_absolute_path,
                                   const std::string& runtime_directory_absolute_path) {
  int res = prepare_to_start(app_directory_absolute_path, runtime_directory_absolute_path);
  if (res == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  const std::string config_absolute_path =
      common::file_system::make_path(app_directory_absolute_path, CONFIG_FILE_NAME);
  if (!common::file_system::is_valid_path(config_absolute_path)) {
    std::cout << "Can't config file path: " << config_absolute_path << std::endl;
    return EXIT_FAILURE;
  }

  fasto::fastotv::client::TVConfig main_options;
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

  AVDictionary* sws_dict = NULL;
  AVDictionary* swr_opts = NULL;
  AVDictionary* format_opts = NULL;
  AVDictionary* codec_opts = NULL;
  av_dict_set(&sws_dict, "flags", "bicubic", 0);

  fasto::fastotv::client::core::ComplexOptions copt(swr_opts, sws_dict, format_opts, codec_opts);
  fasto::fastotv::client::ISimplePlayer* player = new fasto::fastotv::client::SimplePlayer(main_options.player_options);
  player->SetUrlLocation("0", stream_url, main_options.app_options, copt);
  res = app.Exec();
  destroy(&player);

  av_dict_free(&swr_opts);
  av_dict_free(&sws_dict);
  av_dict_free(&format_opts);
  av_dict_free(&codec_opts);
  return res;
}

int main_single_application(int argc,
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
    std::cout << "Can't config file path: " << config_absolute_path << std::endl;
    return EXIT_FAILURE;
  }

  fasto::fastotv::client::TVConfig main_options;
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

  AVDictionary* sws_dict = NULL;
  AVDictionary* swr_opts = NULL;
  AVDictionary* format_opts = NULL;
  AVDictionary* codec_opts = NULL;
  av_dict_set(&sws_dict, "flags", "bicubic", 0);

  fasto::fastotv::client::core::ComplexOptions copt(swr_opts, sws_dict, format_opts, codec_opts);
  fasto::fastotv::client::ISimplePlayer* player = new fasto::fastotv::client::Player(
      app_directory_absolute_path, main_options.player_options, main_options.app_options, copt);
  res = app.Exec();
  main_options.player_options = player->GetOptions();
  destroy(&player);

  av_dict_free(&swr_opts);
  av_dict_free(&sws_dict);
  av_dict_free(&format_opts);
  av_dict_free(&codec_opts);

  err = lock_pid_file.Unlock();
  if (err && err->IsError()) {
    WARNING_LOG() << "Can't unlock pid file path: " << pid_absolute_path;
  }

  err = lock_pid_file.Close();
  if (err && err->IsError()) {
    WARNING_LOG() << "Can't close pid file path: " << pid_absolute_path << ", error: " << err->GetDescription();
  }

  err = common::file_system::remove_file(pid_absolute_path);
  if (err && err->IsError()) {
    WARNING_LOG() << "Can't remove file: " << pid_absolute_path << ", error: " << err->GetDescription();
  }

  // save config file
  err = save_config_file(config_absolute_path, &main_options);
  if (main_options.power_off_on_exit) {
    common::Error err_shut = common::system::Shutdown(common::system::SHUTDOWN);
    if (err_shut && err_shut->IsError()) {
      WARNING_LOG() << "Can't shutdown error: " << err_shut->GetDescription();
    }
  }
  return res;
}
