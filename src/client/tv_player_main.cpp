/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

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

#include <string.h>  // for strcmp, NULL

#include <iostream>

extern "C" {
#include <libavdevice/avdevice.h>  // for avdevice_register_all
#include <libavfilter/avfilter.h>  // for avfilter_register_all
}

#include <common/file_system/file.h>               // for File, create_directory
#include <common/file_system/string_path_utils.h>  // for File, create_directory
#include <common/system/system.h>

#include <player/ffmpeg_application.h>

#include "client/cmdutils.h"  // for DictionaryOptions, show_...
#include "client/load_config.h"
#include "client/player.h"  // for Player

void init_ffmpeg() {
/* register all codecs, demux and protocols */
#if CONFIG_AVDEVICE
  avdevice_register_all();
#endif
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
#if CONFIG_AVFILTER
  avfilter_register_all();
#endif
  av_register_all();
#endif
}

int main_application(int argc, char** argv, const std::string& app_directory_absolute_path) {
  int res = fastoplayer::prepare_to_start(app_directory_absolute_path);
  if (res == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  const std::string config_absolute_path =
      common::file_system::make_path(app_directory_absolute_path, CONFIG_FILE_NAME);
  if (!common::file_system::is_valid_path(config_absolute_path)) {
    std::cout << "Invalid config file path: " << config_absolute_path << std::endl;
    return EXIT_FAILURE;
  }

  fastoplayer::TVConfig main_options;
  common::ErrnoError err = fastotv::client::load_config_file(config_absolute_path, &main_options);
  if (err) {
    return EXIT_FAILURE;
  }

#if defined(LOG_TO_FILE)
  const std::string log_path = common::file_system::make_path(app_directory_absolute_path, std::string(LOG_FILE_NAME));
  INIT_LOGGER(PROJECT_NAME_TITLE, log_path, main_options.loglevel);
#else
  INIT_LOGGER(PROJECT_NAME_TITLE, main_options.loglevel);
#endif

  fastoplayer::FFmpegApplication app(argc, argv);

  AVDictionary* sws_dict = nullptr;
  AVDictionary* swr_opts = nullptr;
  AVDictionary* format_opts = nullptr;
  AVDictionary* codec_opts = nullptr;
  av_dict_set(&sws_dict, "flags", "bicubic", 0);

  fastoplayer::media::ComplexOptions copt(swr_opts, sws_dict, format_opts, codec_opts);
  auto player = new fastotv::client::Player(app_directory_absolute_path, main_options.player_options,
                                            main_options.app_options, copt);
  res = app.Exec();
  main_options.player_options = player->GetOptions();
  destroy(&player);

  av_dict_free(&swr_opts);
  av_dict_free(&sws_dict);
  av_dict_free(&format_opts);
  av_dict_free(&codec_opts);

  // save config file
  err = fastotv::client::save_config_file(config_absolute_path, &main_options);
  if (main_options.power_off_on_exit) {
    common::ErrnoError err_shut = common::system::Shutdown(common::system::SHUTDOWN);
    if (err_shut) {
      WARNING_LOG() << "Can't shutdown error: " << err_shut->GetDescription();
    }
  }
  return res;
}

/* Called from the main */
int main(int argc, char** argv) {
  init_ffmpeg();

  for (int i = 1; i < argc; ++i) {
    const bool lastarg = i == argc - 1;
    if (strcmp(argv[i], "-version") == 0 || strcmp(argv[i], "-v") == 0) {
      show_version();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0) {
      std::string topic;
      if (!lastarg) {
        topic = argv[++i];
      }
      show_help_tv_player(topic);
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-license") == 0 || strcmp(argv[i], "-l") == 0) {
      show_license();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-buildconf") == 0) {
      show_buildconf();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-formats") == 0) {
      show_formats();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-devices") == 0) {
      show_devices();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-muxers") == 0) {
      show_muxers();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-demuxers") == 0) {
      show_demuxers();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-codecs") == 0) {
      show_codecs();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-hwaccels") == 0) {
      show_hwaccels();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-decoders") == 0) {
      show_decoders();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-encoders") == 0) {
      show_encoders();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-bsfs") == 0) {
      show_bsfs();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-protocols") == 0) {
      show_protocols();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-filters") == 0) {
      show_filters();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-pix_fmts") == 0) {
      show_pix_fmts();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-layouts") == 0) {
      show_layouts();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-sample_fmts") == 0) {
      show_sample_fmts();
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-colors") == 0) {
      show_colors();
      return EXIT_SUCCESS;
    }
#if CONFIG_AVDEVICE
    else if (strcmp(argv[i], "-sources") == 0) {
      std::string device;
      if (!lastarg) {
        device = argv[++i];
      }
      show_sources(device);
      return EXIT_SUCCESS;
    } else if (strcmp(argv[i], "-sinks") == 0) {
      std::string device;
      if (!lastarg) {
        device = argv[++i];
      }
      show_sinks(device);
      return EXIT_SUCCESS;
    }
#endif
    else {
      show_help_tv_player(std::string());
      return EXIT_SUCCESS;
    }
  }

  const std::string app_directory_path = APPLICATION_DIR;
  const std::string app_directory_absolute_path =
      common::file_system::is_absolute_path(app_directory_path)
          ? app_directory_path
          : common::file_system::absolute_path_from_relative(app_directory_path);
  return main_application(argc, argv, app_directory_absolute_path);
}
