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

#include <string.h>  // for strcmp, NULL

extern "C" {
#include <libavdevice/avdevice.h>  // for avdevice_register_all
#include <libavfilter/avfilter.h>  // for avfilter_register_all
}

#include <common/file_system.h>  // for File, create_directory

#include "client/cmdutils.h"  // for DictionaryOptions, show_...
#include "client/main_wrapper.h"

void init_ffmpeg() {
  init_dynload();

/* register all codecs, demux and protocols */
#if CONFIG_AVDEVICE
  avdevice_register_all();
#endif
#if CONFIG_AVFILTER
  avfilter_register_all();
#endif
  av_register_all();
}

/* Called from the main */
int main(int argc, char** argv) {
  init_ffmpeg();

  bool is_simple_player_mode = false;
  common::uri::Uri url;

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
      show_help(topic);
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
    } else if (strcmp(argv[i], "-i") == 0 && !lastarg) {
      is_simple_player_mode = true;
      url = common::uri::Uri(argv[++i]);
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
      show_help(std::string());
      return EXIT_SUCCESS;
    }
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
  if (is_simple_player_mode) {
    return main_simple_player_application(argc, argv, url, app_directory_absolute_path,
                                          runtime_directory_absolute_path);
  }
  return main_application(argc, argv, app_directory_absolute_path, runtime_directory_absolute_path, false);
}
