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

#include "client/player/ffmpeg_application.h"

#include <signal.h>

#include <iostream>

extern "C" {
#include <libavformat/avformat.h>  // for av_register_all, avforma...
}

#include <common/file_system.h>

#include "client/player/events/events.h"

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

int lockmgr(void** mtx, enum AVLockOp op) {
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

}  // namespace

namespace fastotv {
namespace client {
namespace player {

FFmpegApplication::FFmpegApplication(int argc, char** argv) : base_class_t(argc, argv) {
  avformat_network_init();
  signal(SIGINT, sigterm_handler);  /* Interrupt (ANSI).    */
  signal(SIGTERM, sigterm_handler); /* Termination (ANSI).  */

  av_log_set_callback(avlog_cb);
  int ffmpeg_log_level = fasto_log_to_ffmpeg(common::logging::CURRENT_LOG_LEVEL());
  av_log_set_level(ffmpeg_log_level);
}

FFmpegApplication::~FFmpegApplication() {
  av_lockmgr_register(NULL);
  avformat_network_deinit();
}

int FFmpegApplication::PreExecImpl() {
  if (av_lockmgr_register(lockmgr)) {
    ERROR_LOG() << "Could not initialize lock manager!";
    return EXIT_FAILURE;
  }

  int pre_exec = base_class_t::PreExecImpl();
  core::events::PreExecInfo inf(pre_exec);
  core::events::PreExecEvent* pre_event = new core::events::PreExecEvent(this, inf);
  base_class_t::SendEvent(pre_event);
  return pre_exec;
}

int FFmpegApplication::PostExecImpl() {
  core::events::PostExecInfo inf(EXIT_SUCCESS);
  core::events::PostExecEvent* post_event = new core::events::PostExecEvent(this, inf);
  base_class_t::SendEvent(post_event);
  return base_class_t::PostExecImpl();
}

int prepare_to_start(const std::string& app_directory_absolute_path) {
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

  return EXIT_SUCCESS;
}

}  // namespace player
}  // namespace client
}  // namespace fastotv
