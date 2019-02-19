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

#include "client/load_config.h"

#include <string.h>  // for strcmp

#include <limits>
#include <string>
#include <vector>

#include <common/file_system/file.h>
#include <common/file_system/string_path_utils.h>
#include <common/utils.h>

#include <player/media/ffmpeg_internal.h>  // for HWAccelID

#include "inih/ini.h"  // for ini_parse

#include "client/cmdutils.h"
#include "client/types.h"  // for Size

#define CONFIG_MAIN_OPTIONS "main_options"
#define CONFIG_MAIN_OPTIONS_LOG_LEVEL_FIELD "loglevel"
#define CONFIG_MAIN_OPTIONS_POWEROFF_ON_EXIT_FIELD "poweroffonexit"

#define CONFIG_PLAYER_OPTIONS "player_options"
#define CONFIG_PLAYER_OPTIONS_WIDTH_FIELD "width"
#define CONFIG_PLAYER_OPTIONS_HEIGHT_FIELD "height"
#define CONFIG_PLAYER_OPTIONS_FULLSCREEN_FIELD "fullscreen"
#define CONFIG_PLAYER_OPTIONS_VOLUME_FIELD "volume"
#define CONFIG_PLAYER_OPTIONS_LAST_SHOWED_CHANNEL_ID_FIELD "last_showed_channel_id"

#define CONFIG_APP_OPTIONS "app_options"
#define CONFIG_APP_OPTIONS_AST_FIELD "ast"
#define CONFIG_APP_OPTIONS_VST_FIELD "vst"
#define CONFIG_APP_OPTIONS_FAST_FIELD "fast"
#define CONFIG_APP_OPTIONS_GENPTS_FIELD "genpts"
#define CONFIG_APP_OPTIONS_LOWRES_FIELD "lowres"
#define CONFIG_APP_OPTIONS_SYNC_FIELD "sync"
#define CONFIG_APP_OPTIONS_FRAMEDROP_FIELD "framedrop"
#define CONFIG_APP_OPTIONS_BYTES_FIELD "bytes"
#define CONFIG_APP_OPTIONS_INFBUF_FIELD "infbuf"
#define CONFIG_APP_OPTIONS_VF_FIELD "vf"
#define CONFIG_APP_OPTIONS_AF_FIELD "af"
#define CONFIG_APP_OPTIONS_VN_FIELD "vn"
#define CONFIG_APP_OPTIONS_AN_FIELD "an"
#define CONFIG_APP_OPTIONS_ACODEC_FIELD "acodec"
#define CONFIG_APP_OPTIONS_VCODEC_FIELD "vcodec"
#define CONFIG_APP_OPTIONS_HWACCEL_FIELD "hwaccel"
#define CONFIG_APP_OPTIONS_HWACCEL_DEVICE_FIELD "hwaccel_device"
#define CONFIG_APP_OPTIONS_HWACCEL_OUTPUT_FORMAT_FIELD "hwaccel_output_format"
#define CONFIG_APP_OPTIONS_AUTOROTATE_FIELD "autorotate"

// vaapi args: -hwaccel vaapi -hwaccel_device /dev/dri/card0
// vdpau args: -hwaccel vdpau
// scale output: -vf scale=1920x1080
// deinterlace output: -vf yadif

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
*/

namespace fastotv {
namespace client {

namespace {

int ini_handler_fasto(void* user, const char* section, const char* name, const char* value) {
  fastoplayer::TVConfig* pconfig = reinterpret_cast<fastoplayer::TVConfig*>(user);
  size_t value_len = strlen(value);
  if (value_len == 0) {  // skip empty fields
    return 0;
  }

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
  if (MATCH(CONFIG_MAIN_OPTIONS, CONFIG_MAIN_OPTIONS_LOG_LEVEL_FIELD)) {
    common::logging::LOG_LEVEL lg;
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
  } else if (MATCH(CONFIG_PLAYER_OPTIONS, CONFIG_PLAYER_OPTIONS_LAST_SHOWED_CHANNEL_ID_FIELD)) {
    pconfig->player_options.last_showed_channel_id = value;
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_AST_FIELD)) {
    pconfig->app_options.wanted_stream_spec[AVMEDIA_TYPE_AUDIO] = value;
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_VST_FIELD)) {
    pconfig->app_options.wanted_stream_spec[AVMEDIA_TYPE_VIDEO] = value;
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
    if (parse_number(value, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), &lowres)) {
      pconfig->app_options.lowres = lowres;
    }
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_SYNC_FIELD)) {
    if (strcmp(value, "audio") == 0) {
      pconfig->app_options.av_sync_type = fastoplayer::media::AV_SYNC_AUDIO_MASTER;
    } else if (strcmp(value, "video") == 0) {
      pconfig->app_options.av_sync_type = fastoplayer::media::AV_SYNC_VIDEO_MASTER;
    } else {
      return 0;
    }
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_FRAMEDROP_FIELD)) {
    if (strcmp(value, "auto") == 0) {
      pconfig->app_options.framedrop = fastoplayer::media::FRAME_DROP_AUTO;
    } else if (strcmp(value, "off") == 0) {
      pconfig->app_options.framedrop = fastoplayer::media::FRAME_DROP_OFF;
    } else if (strcmp(value, "on") == 0) {
      pconfig->app_options.framedrop = fastoplayer::media::FRAME_DROP_ON;
    } else {
      return 0;
    }

    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_BYTES_FIELD)) {
    if (strcmp(value, "auto") == 0) {
      pconfig->app_options.seek_by_bytes = fastoplayer::media::SEEK_AUTO;
    } else if (strcmp(value, "off") == 0) {
      pconfig->app_options.seek_by_bytes = fastoplayer::media::SEEK_BY_BYTES_OFF;
    } else if (strcmp(value, "on") == 0) {
      pconfig->app_options.seek_by_bytes = fastoplayer::media::SEEK_BY_BYTES_ON;
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
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_VN_FIELD)) {
    bool disable_video;
    if (parse_bool(value, &disable_video)) {
      pconfig->app_options.enable_video = !disable_video;
    }
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_AN_FIELD)) {
    bool disable_audio;
    if (parse_bool(value, &disable_audio)) {
      pconfig->app_options.enable_audio = !disable_audio;
    }
    return 1;
#if CONFIG_AVFILTER
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_VF_FIELD)) {
    std::vector<std::string> tokens;
    size_t vf_count = common::Tokenize(value, ",", &tokens);
    for (size_t i = 0; i < vf_count; ++i) {
      std::string arg_copy = tokens[i];
      size_t del = arg_copy.find_first_of('=');
      if (del != std::string::npos) {
        std::string key = arg_copy.substr(0, del);
        std::string value = arg_copy.substr(del + 1);
        if (key == "scale") {
          common::draw::Size sz;
          if (common::ConvertFromString(value, &sz)) {
            pconfig->player_options.screen_size = sz;
          }
        }
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
    pconfig->app_options.video_codec_name = value;
    return 1;
  } else if (MATCH(CONFIG_APP_OPTIONS, CONFIG_APP_OPTIONS_HWACCEL_FIELD)) {
    fastoplayer::media::HWAccelID hwid;
    fastoplayer::media::HWDeviceType dtype;
    if (fastoplayer::media::HWAccelIDFromString(value, &hwid, &dtype)) {
      pconfig->app_options.hwaccel_id = hwid;
      pconfig->app_options.hwaccel_device_type = dtype;
    }
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
  } else {
    return 0; /* unknown section/name, error */
  }
}
}  // namespace

common::ErrnoError load_config_file(const std::string& config_absolute_path, fastoplayer::TVConfig* options) {
  if (!options) {
    return common::make_errno_error_inval();
  }

  std::string copy_config_absolute_path = config_absolute_path;
  if (!common::file_system::is_file_exist(config_absolute_path)) {
    const std::string absolute_source_dir = common::file_system::absolute_path_from_relative(RELATIVE_SOURCE_DIR);
    copy_config_absolute_path = common::file_system::make_path(absolute_source_dir, CONFIG_FILE_PATH_RELATIVE);
  }

  if (!copy_config_absolute_path.empty()) {
    ini_parse(copy_config_absolute_path.c_str(), ini_handler_fasto, options);
  }
  return common::ErrnoError();
}

common::ErrnoError save_config_file(const std::string& config_absolute_path, fastoplayer::TVConfig* options) {
  if (!options || config_absolute_path.empty()) {
    return common::make_errno_error_inval();
  }

  common::file_system::FileGuard<common::file_system::ANSIFile> config_save_file;
  common::ErrnoError err = config_save_file.Open(config_absolute_path, "w");
  if (err) {
    return err;
  }

  config_save_file.Write("[" CONFIG_MAIN_OPTIONS "]\n");
  config_save_file.WriteFormated(CONFIG_MAIN_OPTIONS_LOG_LEVEL_FIELD "=%s\n",
                                 common::logging::log_level_to_text(options->loglevel));
  config_save_file.WriteFormated(CONFIG_MAIN_OPTIONS_POWEROFF_ON_EXIT_FIELD "=%s\n",
                                 common::ConvertToString(options->power_off_on_exit));

  config_save_file.Write("[" CONFIG_APP_OPTIONS "]\n");
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_AST_FIELD "=%s\n",
                                 options->app_options.wanted_stream_spec[AVMEDIA_TYPE_AUDIO]);
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_VST_FIELD "=%s\n",
                                 options->app_options.wanted_stream_spec[AVMEDIA_TYPE_VIDEO]);
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_FAST_FIELD "=%s\n",
                                 common::ConvertToString(options->app_options.fast));
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_GENPTS_FIELD "=%s\n",
                                 common::ConvertToString(options->app_options.genpts));
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_LOWRES_FIELD "=%d\n", options->app_options.lowres);
  config_save_file.WriteFormated(
      CONFIG_APP_OPTIONS_SYNC_FIELD "=%s\n",
      options->app_options.av_sync_type == fastoplayer::media::AV_SYNC_AUDIO_MASTER ? "audio" : "video");
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_FRAMEDROP_FIELD "=%d\n",
                                 static_cast<int>(options->app_options.framedrop));
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_BYTES_FIELD "=%d\n",
                                 static_cast<int>(options->app_options.seek_by_bytes));
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_INFBUF_FIELD "=%d\n", options->app_options.infinite_buffer);

  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_VN_FIELD "=%s\n",
                                 common::ConvertToString(!options->app_options.enable_video));
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_AN_FIELD "=%s\n",
                                 common::ConvertToString(!options->app_options.enable_audio));
#if CONFIG_AVFILTER
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_VF_FIELD "=%s\n", options->app_options.vfilters);
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_AF_FIELD "=%s\n", options->app_options.afilters);
#endif
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_ACODEC_FIELD "=%s\n", options->app_options.audio_codec_name);
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_VCODEC_FIELD "=%s\n", options->app_options.video_codec_name);
  config_save_file.WriteFormated(
      CONFIG_APP_OPTIONS_HWACCEL_FIELD "=%s\n",
      fastoplayer::media::HWAccelIDToString(options->app_options.hwaccel_id, options->app_options.hwaccel_device_type));
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_HWACCEL_DEVICE_FIELD "=%s\n", options->app_options.hwaccel_device);
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_HWACCEL_OUTPUT_FORMAT_FIELD "=%s\n",
                                 options->app_options.hwaccel_output_format);
  config_save_file.WriteFormated(CONFIG_APP_OPTIONS_AUTOROTATE_FIELD "=%s\n",
                                 common::ConvertToString(options->app_options.autorotate));

  config_save_file.Write("[" CONFIG_PLAYER_OPTIONS "]\n");
  config_save_file.WriteFormated(CONFIG_PLAYER_OPTIONS_WIDTH_FIELD "=%d\n", options->player_options.screen_size.width);
  config_save_file.WriteFormated(CONFIG_PLAYER_OPTIONS_HEIGHT_FIELD "=%d\n",
                                 options->player_options.screen_size.height);
  config_save_file.WriteFormated(CONFIG_PLAYER_OPTIONS_FULLSCREEN_FIELD "=%s\n",
                                 common::ConvertToString(options->player_options.is_full_screen));
  config_save_file.WriteFormated(CONFIG_PLAYER_OPTIONS_VOLUME_FIELD "=%d\n", options->player_options.audio_volume);
  config_save_file.WriteFormated(CONFIG_PLAYER_OPTIONS_LAST_SHOWED_CHANNEL_ID_FIELD "=%s\n",
                                 options->player_options.last_showed_channel_id);
  return common::ErrnoError();
}
}  // namespace client
}  // namespace fastotv
