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

#include "client/player.h"

#include <stdlib.h>  // for NULL, EXIT_FAILURE, EXIT...
#include <string.h>  // for memset

#include <SDL2/SDL_audio.h>  // for SDL_CloseAudio, SDL_MIX_...
#include <SDL2/SDL_hints.h>  // for SDL_SetHint, SDL_HINT_RE...
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_pixels.h>   // for SDL_Color, ::SDL_PIXELFO...
#include <SDL2/SDL_rect.h>     // for SDL_Rect
#include <SDL2/SDL_surface.h>  // for SDL_Surface, SDL_FreeSur...

#include <common/application/application.h>  // for fApp, Application
#include <common/convert2string.h>
#include <common/file_system.h>
#include <common/logger.h>                  // for COMPACT_LOG_FILE_CRIT
#include <common/macros.h>                  // for UNUSED, NOTREACHED, ERRO...
#include <common/threads/thread_manager.h>  // for THREAD_MANAGER
#include <common/threads/types.h>           // for thread
#include <common/utils.h>

extern "C" {
#include <libavutil/avutil.h>  // for AVMediaType::AVMEDIA_TYP...
#include <libavutil/buffer.h>  // for av_buffer_unref
#include <libavutil/pixfmt.h>  // for AVPixelFormat::AV_PIX_FM...
}

#include "client/core/app_options.h"      // for AppOptions, ComplexOptio...
#include "client/core/audio_params.h"     // for AudioParams
#include "client/core/ffmpeg_internal.h"  // for hw_device_ctx
#include "client/core/sdl_utils.h"
#include "client/core/video_state.h"  // for VideoState

#include "client/core/frames/audio_frame.h"  // for AudioFrame
#include "client/core/frames/video_frame.h"  // for VideoFrame

#include "client/ioservice.h"  // for IoService
#include "client/sdl_utils.h"  // for IMG_LoadPNG, SurfaceSaver
#include "client/av_sdl_utils.h"
#include "client/utils.h"

#include "channel_info.h"  // for Url

/* Step size for volume control */
#define VOLUME_STEP 1

#define CURSOR_HIDE_DELAY_MSEC 1000  // 1 sec
#define FOOTER_HIDE_DELAY_MSEC 2000  // 1 sec
#define VOLUME_HIDE_DELAY_MSEC 2000  // 2 sec

#define USER_FIELD "user"
#define URLS_FIELD "urls"

#define IMG_OFFLINE_CHANNEL_PATH_RELATIVE "share/resources/offline_channel.png"
#define IMG_CONNECTION_ERROR_PATH_RELATIVE "share/resources/connection_error.png"
#define MAIN_FONT_PATH_RELATIVE "share/fonts/FreeSans.ttf"

#define CACHE_FOLDER_NAME "cache"

namespace {
int CalcHeightFontPlaceByRowCount(const TTF_Font* font, int row) {
  if (!font) {
    return 0;
  }

  int font_height = TTF_FontLineSkip(font);
  return 2 << av_log2(font_height * row);
}
}  // namespace

namespace fasto {
namespace fastotv {
namespace client {

const SDL_Color Player::text_color = {255, 255, 255, 0};
const AVRational Player::min_fps = {25, 1};

Player::Player(const std::string& app_directory_absolute_path,
               const PlayerOptions& options,
               const core::AppOptions& opt,
               const core::ComplexOptions& copt)
    : StreamHandler(),
      options_(options),
      opt_(opt),
      copt_(copt),
      play_list_(),
      audio_params_(nullptr),
      audio_buff_size_(0),
      renderer_(NULL),
      window_(NULL),
      show_cursor_(false),
      cursor_last_shown_(0),
      show_footer_(false),
      footer_last_shown_(0),
      show_volume_(false),
      volume_last_shown_(0),
      last_mouse_left_click_(0),
      current_stream_pos_(0),
      offline_channel_texture_(nullptr),
      connection_error_texture_(nullptr),
      font_(NULL),
      stream_(nullptr),
      window_size_(),
      xleft_(0),
      ytop_(0),
      controller_(new IoService),
      current_state_(INIT_STATE),
      current_state_str_("Init"),
      muted_(false),
      show_statstic_(false),
      app_directory_absolute_path_(app_directory_absolute_path),
      render_texture_(NULL),
      update_video_timer_interval_msec_(0),
      last_pts_checkpoint_(core::invalid_clock()),
      video_frames_handled_(0) {
  UpdateDisplayInterval(min_fps);
  // stable audio option
  options_.audio_volume = stable_value_in_range(options_.audio_volume, 0, 100);

  fApp->Subscribe(this, core::events::PostExecEvent::EventType);
  fApp->Subscribe(this, core::events::PreExecEvent::EventType);
  fApp->Subscribe(this, core::events::TimerEvent::EventType);

  fApp->Subscribe(this, core::events::RequestVideoEvent::EventType);
  fApp->Subscribe(this, core::events::QuitStreamEvent::EventType);

  fApp->Subscribe(this, core::events::KeyPressEvent::EventType);

  fApp->Subscribe(this, core::events::LircPressEvent::EventType);

  fApp->Subscribe(this, core::events::MouseMoveEvent::EventType);
  fApp->Subscribe(this, core::events::MousePressEvent::EventType);

  fApp->Subscribe(this, core::events::WindowResizeEvent::EventType);
  fApp->Subscribe(this, core::events::WindowExposeEvent::EventType);
  fApp->Subscribe(this, core::events::WindowCloseEvent::EventType);

  fApp->Subscribe(this, core::events::QuitEvent::EventType);

  fApp->Subscribe(this, core::events::ClientDisconnectedEvent::EventType);
  fApp->Subscribe(this, core::events::ClientConnectedEvent::EventType);

  fApp->Subscribe(this, core::events::ClientAuthorizedEvent::EventType);
  fApp->Subscribe(this, core::events::ClientUnAuthorizedEvent::EventType);

  fApp->Subscribe(this, core::events::ClientConfigChangeEvent::EventType);
  fApp->Subscribe(this, core::events::ReceiveChannelsEvent::EventType);
  fApp->Subscribe(this, core::events::BandwidthEstimationEvent::EventType);
}

void Player::SetFullScreen(bool full_screen) {
  options_.is_full_screen = full_screen;
  SDL_SetWindowFullscreen(window_, full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  if (stream_) {
    stream_->RefreshRequest();
  }
}

void Player::SetMute(bool mute) {
  muted_ = mute;
}

Player::~Player() {
  if (core::hw_device_ctx) {
    av_buffer_unref(&core::hw_device_ctx);
    core::hw_device_ctx = NULL;
  }
  destroy(&controller_);
  fApp->UnSubscribe(this);
}

PlayerOptions Player::GetOptions() const {
  return options_;
}

void Player::HandleEvent(event_t* event) {
  if (event->GetEventType() == core::events::PreExecEvent::EventType) {
    core::events::PreExecEvent* pre_event = static_cast<core::events::PreExecEvent*>(event);
    HandlePreExecEvent(pre_event);
  } else if (event->GetEventType() == core::events::PostExecEvent::EventType) {
    core::events::PostExecEvent* post_event = static_cast<core::events::PostExecEvent*>(event);
    HandlePostExecEvent(post_event);
  } else if (event->GetEventType() == core::events::RequestVideoEvent::EventType) {
    core::events::RequestVideoEvent* avent = static_cast<core::events::RequestVideoEvent*>(event);
    HandleRequestVideoEvent(avent);
  } else if (event->GetEventType() == core::events::QuitStreamEvent::EventType) {
    core::events::QuitStreamEvent* quit_stream_event = static_cast<core::events::QuitStreamEvent*>(event);
    HandleQuitStreamEvent(quit_stream_event);
  } else if (event->GetEventType() == core::events::TimerEvent::EventType) {
    core::events::TimerEvent* tevent = static_cast<core::events::TimerEvent*>(event);
    HandleTimerEvent(tevent);
  } else if (event->GetEventType() == core::events::KeyPressEvent::EventType) {
    core::events::KeyPressEvent* key_press_event = static_cast<core::events::KeyPressEvent*>(event);
    HandleKeyPressEvent(key_press_event);
  } else if (event->GetEventType() == core::events::LircPressEvent::EventType) {
    core::events::LircPressEvent* lirc_press_event = static_cast<core::events::LircPressEvent*>(event);
    HandleLircPressEvent(lirc_press_event);
  } else if (event->GetEventType() == core::events::WindowResizeEvent::EventType) {
    core::events::WindowResizeEvent* win_resize_event = static_cast<core::events::WindowResizeEvent*>(event);
    HandleWindowResizeEvent(win_resize_event);
  } else if (event->GetEventType() == core::events::WindowExposeEvent::EventType) {
    core::events::WindowExposeEvent* win_expose = static_cast<core::events::WindowExposeEvent*>(event);
    HandleWindowExposeEvent(win_expose);
  } else if (event->GetEventType() == core::events::WindowCloseEvent::EventType) {
    core::events::WindowCloseEvent* window_close = static_cast<core::events::WindowCloseEvent*>(event);
    HandleWindowCloseEvent(window_close);
  } else if (event->GetEventType() == core::events::MouseMoveEvent::EventType) {
    core::events::MouseMoveEvent* mouse_move = static_cast<core::events::MouseMoveEvent*>(event);
    HandleMouseMoveEvent(mouse_move);
  } else if (event->GetEventType() == core::events::MousePressEvent::EventType) {
    core::events::MousePressEvent* mouse_press = static_cast<core::events::MousePressEvent*>(event);
    HandleMousePressEvent(mouse_press);
  } else if (event->GetEventType() == core::events::QuitEvent::EventType) {
    core::events::QuitEvent* quit_event = static_cast<core::events::QuitEvent*>(event);
    HandleQuitEvent(quit_event);
  } else if (event->GetEventType() == core::events::ClientConnectedEvent::EventType) {
    core::events::ClientConnectedEvent* connect_event = static_cast<core::events::ClientConnectedEvent*>(event);
    HandleClientConnectedEvent(connect_event);
  } else if (event->GetEventType() == core::events::ClientDisconnectedEvent::EventType) {
    core::events::ClientDisconnectedEvent* disc_event = static_cast<core::events::ClientDisconnectedEvent*>(event);
    HandleClientDisconnectedEvent(disc_event);
  } else if (event->GetEventType() == core::events::ClientAuthorizedEvent::EventType) {
    core::events::ClientAuthorizedEvent* auth_event = static_cast<core::events::ClientAuthorizedEvent*>(event);
    HandleClientAuthorizedEvent(auth_event);
  } else if (event->GetEventType() == core::events::ClientUnAuthorizedEvent::EventType) {
    core::events::ClientUnAuthorizedEvent* unauth_event = static_cast<core::events::ClientUnAuthorizedEvent*>(event);
    HandleClientUnAuthorizedEvent(unauth_event);
  } else if (event->GetEventType() == core::events::ClientConfigChangeEvent::EventType) {
    core::events::ClientConfigChangeEvent* conf_change_event =
        static_cast<core::events::ClientConfigChangeEvent*>(event);
    HandleClientConfigChangeEvent(conf_change_event);
  } else if (event->GetEventType() == core::events::ReceiveChannelsEvent::EventType) {
    core::events::ReceiveChannelsEvent* channels_event = static_cast<core::events::ReceiveChannelsEvent*>(event);
    HandleReceiveChannelsEvent(channels_event);
  } else if (event->GetEventType() == core::events::BandwidthEstimationEvent::EventType) {
    core::events::BandwidthEstimationEvent* band_event = static_cast<core::events::BandwidthEstimationEvent*>(event);
    HandleBandwidthEstimationEvent(band_event);
  }
}

void Player::HandleExceptionEvent(event_t* event, common::Error err) {
  UNUSED(err);
  if (event->GetEventType() == core::events::ClientConnectedEvent::EventType) {
    // core::events::ClientConnectedEvent* connect_event =
    //    static_cast<core::events::ClientConnectedEvent*>(event);
    SwitchToDisconnectMode();
  } else if (event->GetEventType() == core::events::ClientAuthorizedEvent::EventType) {
    // core::events::ClientConnectedEvent* connect_event =
    //    static_cast<core::events::ClientConnectedEvent*>(event);
    SwitchToUnAuthorizeMode();
  } else if (event->GetEventType() == core::events::BandwidthEstimationEvent::EventType) {
    core::events::BandwidthEstimationEvent* band_event = static_cast<core::events::BandwidthEstimationEvent*>(event);
    HandleBandwidthEstimationEvent(band_event);
  }
}

bool Player::HandleRequestAudio(core::VideoState* stream,
                                int64_t wanted_channel_layout,
                                int wanted_nb_channels,
                                int wanted_sample_rate,
                                core::AudioParams* audio_hw_params,
                                int* audio_buff_size) {
  UNUSED(stream);

  if (audio_params_) {
    *audio_hw_params = *audio_params_;
    *audio_buff_size = audio_buff_size_;
    return true;
  }

  /* prepare audio output */
  core::AudioParams laudio_hw_params;
  int laudio_buff_size;
  if (!core::audio_open(this, wanted_channel_layout, wanted_nb_channels, wanted_sample_rate, sdl_audio_callback,
                        &laudio_hw_params, &laudio_buff_size)) {
    return false;
  }

  SDL_PauseAudio(0);
  audio_params_ = new core::AudioParams(laudio_hw_params);
  audio_buff_size_ = laudio_buff_size;

  *audio_hw_params = *audio_params_;
  *audio_buff_size = audio_buff_size_;
  return true;
}

void Player::HanleAudioMix(uint8_t* audio_stream_ptr, const uint8_t* src, uint32_t len, int volume) {
  SDL_MixAudio(audio_stream_ptr, src, len, ConvertToSDLVolume(volume));
}

bool Player::HandleRequestVideo(core::VideoState* stream,
                                int width,
                                int height,
                                int av_pixel_format,
                                AVRational aspect_ratio) {
  UNUSED(av_pixel_format);
  CHECK(THREAD_MANAGER()->IsMainThread());
  if (!stream) {  // invalid input
    return false;
  }

  SDL_Rect rect = core::calculate_display_rect(0, 0, INT_MAX, height, width, height, aspect_ratio);
  options_.default_size.width = rect.w;
  options_.default_size.height = rect.h;

  InitWindow(GetCurrentUrlName(), PLAYING_STATE);

  AVRational frame_rate = stream->GetFrameRate();
  UpdateDisplayInterval(frame_rate);
  return true;
}

void Player::HandleRequestVideoEvent(core::events::RequestVideoEvent* event) {
  core::events::RequestVideoEvent* avent = static_cast<core::events::RequestVideoEvent*>(event);
  core::events::FrameInfo fr = avent->info();
  bool res = fr.stream_->RequestVideo(fr.width, fr.height, fr.av_pixel_format, fr.aspect_ratio);
  if (res) {
    return;
  }

  if (stream_) {
    stream_->Abort();
    destroy(&stream_);
  }
  Quit();
}

void Player::HandleQuitStreamEvent(core::events::QuitStreamEvent* event) {
  core::events::QuitStreamInfo inf = event->info();
  if (inf.stream_ && inf.stream_->IsAborted()) {
    return;
  }

  stream_->Abort();
  destroy(&stream_);
  SwitchToChannelErrorMode(inf.error);
}

void Player::HandlePreExecEvent(core::events::PreExecEvent* event) {
  core::events::PreExecInfo inf = event->info();
  if (inf.code == EXIT_SUCCESS) {
    const std::string absolute_source_dir = common::file_system::absolute_path_from_relative(RELATIVE_SOURCE_DIR);
    const std::string offline_channel_img_full_path =
        common::file_system::make_path(absolute_source_dir, IMG_OFFLINE_CHANNEL_PATH_RELATIVE);
    const char* offline_channel_img_full_path_ptr = common::utils::c_strornull(offline_channel_img_full_path);
    SDL_Surface* surface = IMG_Load(offline_channel_img_full_path_ptr);
    if (surface) {
      offline_channel_texture_ = new SurfaceSaver(surface);
    }

    const std::string connection_error_img_full_path =
        common::file_system::make_path(absolute_source_dir, IMG_CONNECTION_ERROR_PATH_RELATIVE);
    const char* connection_error_img_full_path_ptr = common::utils::c_strornull(connection_error_img_full_path);
    SDL_Surface* surface2 = IMG_Load(connection_error_img_full_path_ptr);
    if (surface2) {
      connection_error_texture_ = new SurfaceSaver(surface2);
    }

    render_texture_ = new TextureSaver;

    const std::string font_path = common::file_system::make_path(absolute_source_dir, MAIN_FONT_PATH_RELATIVE);
    const char* font_path_ptr = common::utils::c_strornull(font_path);
    font_ = TTF_OpenFont(font_path_ptr, 24);
    if (!font_) {
      WARNING_LOG() << "Couldn't open font file path: " << font_path;
    }
    controller_->Start();
    SwitchToConnectMode();
  }
}

void Player::HandlePostExecEvent(core::events::PostExecEvent* event) {
  core::events::PostExecInfo inf = event->info();
  if (inf.code == EXIT_SUCCESS) {
    controller_->Stop();
    if (stream_) {
      stream_->Abort();
      destroy(&stream_);
    }
    if (font_) {
      TTF_CloseFont(font_);
      font_ = NULL;
    }

    play_list_.clear();
    destroy(&offline_channel_texture_);
    destroy(&connection_error_texture_);

    SDL_CloseAudio();
    destroy(&audio_params_);

    destroy(&render_texture_);

    if (renderer_) {
      SDL_DestroyRenderer(renderer_);
      renderer_ = NULL;
    }
    if (window_) {
      SDL_DestroyWindow(window_);
      window_ = NULL;
    }
  } else {
    NOTREACHED();
  }
}

void Player::HandleTimerEvent(core::events::TimerEvent* event) {
  UNUSED(event);
  const core::msec_t cur_time = core::GetCurrentMsec();
  core::msec_t diff_currsor = cur_time - cursor_last_shown_;
  if (show_cursor_ && diff_currsor > CURSOR_HIDE_DELAY_MSEC) {
    fApp->HideCursor();
    show_cursor_ = false;
  }

  core::msec_t diff_footer = cur_time - footer_last_shown_;
  if (show_footer_ && diff_footer > FOOTER_HIDE_DELAY_MSEC) {
    show_footer_ = false;
  }

  core::msec_t diff_volume = cur_time - volume_last_shown_;
  if (show_volume_ && diff_volume > VOLUME_HIDE_DELAY_MSEC) {
    show_volume_ = false;
  }
  DrawDisplay();
}

void Player::HandleLircPressEvent(core::events::LircPressEvent* event) {
  if (options_.exit_on_keydown) {
    Quit();
    return;
  }

  core::events::LircPressInfo inf = event->info();
  switch (inf.code) {
    case LIRC_KEY_OK: {
      PauseStream();
      break;
    }
    case LIRC_KEY_LEFT: {
      MoveToPreviousStream();
      break;
    }
    case LIRC_KEY_UP: {
      UpdateVolume(VOLUME_STEP);
      break;
    }
    case LIRC_KEY_RIGHT: {
      MoveToNextStream();
      break;
    }
    case LIRC_KEY_DOWN: {
      UpdateVolume(-VOLUME_STEP);
      break;
    }
    case LIRC_KEY_EXIT: {
      Quit();
      break;
    }
    case LIRC_KEY_MUTE: {
      ToggleMute();
      break;
    }
    default: { break; }
  }
}

void Player::HandleKeyPressEvent(core::events::KeyPressEvent* event) {
  if (options_.exit_on_keydown) {
    Quit();
    return;
  }

  core::events::KeyPressInfo inf = event->info();
  switch (inf.ks.sym) {
    case FASTO_KEY_ESCAPE:
    case FASTO_KEY_q: {
      Quit();
      return;
    }
    case FASTO_KEY_f: {
      bool full_screen = !options_.is_full_screen;
      SetFullScreen(full_screen);
      break;
    }
    case FASTO_KEY_F3: {
      ToggleStatistic();
      break;
    }
    case FASTO_KEY_F4: {
      StartShowFooter();
      break;
    }
    case FASTO_KEY_p:
    case FASTO_KEY_SPACE:
      PauseStream();
      break;
    case FASTO_KEY_m: {
      ToggleMute();
      break;
    }
    case FASTO_KEY_KP_MULTIPLY:
    case FASTO_KEY_0:
      UpdateVolume(VOLUME_STEP);
      break;
    case FASTO_KEY_KP_DIVIDE:
    case FASTO_KEY_9:
      UpdateVolume(-VOLUME_STEP);
      break;
    case FASTO_KEY_s:  // S: Step to next frame
      if (stream_) {
        stream_->StepToNextFrame();
      }
      break;
    case FASTO_KEY_a:
      if (stream_) {
        stream_->StreamCycleChannel(AVMEDIA_TYPE_AUDIO);
      }
      break;
    case FASTO_KEY_v:
      if (stream_) {
        stream_->StreamCycleChannel(AVMEDIA_TYPE_VIDEO);
      }
      break;
    case FASTO_KEY_c:
      if (stream_) {
        stream_->StreamCycleChannel(AVMEDIA_TYPE_VIDEO);
        stream_->StreamCycleChannel(AVMEDIA_TYPE_AUDIO);
      }
      break;
    case FASTO_KEY_t:
      // StreamCycleChannel(AVMEDIA_TYPE_SUBTITLE);
      break;
    case FASTO_KEY_w: {
      break;
    }
    //
    case FASTO_KEY_PAGEUP:
      if (stream_) {
        stream_->SeekNextChunk();
      }
      break;
    case FASTO_KEY_PAGEDOWN:
      if (stream_) {
        stream_->SeekPrevChunk();
      }
      break;
    case FASTO_KEY_LEFT:
      if (stream_) {
        stream_->Seek(-10000);  // msec
      }
      break;
    case FASTO_KEY_RIGHT:
      if (stream_) {
        stream_->Seek(10000);  // msec
      }
      break;
    case FASTO_KEY_UP:
      if (stream_) {
        stream_->Seek(60000);  // msec
      }
      break;
    case FASTO_KEY_DOWN:
      if (stream_) {
        stream_->Seek(-60000);  // msec
      }
      break;
    case FASTO_KEY_LEFTBRACKET: {
      MoveToPreviousStream();
      break;
    }
    case FASTO_KEY_RIGHTBRACKET: {
      MoveToNextStream();
      break;
    }
    default:
      break;
  }
}

void Player::HandleMousePressEvent(core::events::MousePressEvent* event) {
  if (options_.exit_on_mousedown) {
    Quit();
    return;
  }

  core::msec_t cur_time = core::GetCurrentMsec();
  core::events::MousePressInfo inf = event->info();
  if (inf.button == FASTO_BUTTON_LEFT) {
    if (cur_time - last_mouse_left_click_ <= 500) {  // double click 0.5 sec
      bool full_screen = !options_.is_full_screen;
      SetFullScreen(full_screen);
      last_mouse_left_click_ = 0;
    } else {
      last_mouse_left_click_ = cur_time;
    }
  }

  if (!show_cursor_) {
    fApp->ShowCursor();
    show_cursor_ = true;
  }
  cursor_last_shown_ = cur_time;
}

void Player::HandleMouseMoveEvent(core::events::MouseMoveEvent* event) {
  UNUSED(event);
  if (!show_cursor_) {
    fApp->ShowCursor();
    show_cursor_ = true;
  }
  core::msec_t cur_time = core::GetCurrentMsec();
  cursor_last_shown_ = cur_time;
}

void Player::HandleWindowResizeEvent(core::events::WindowResizeEvent* event) {
  core::events::WindowResizeInfo inf = event->info();
  window_size_ = inf.size;
  if (stream_) {
    stream_->RefreshRequest();
  }
}

void Player::HandleWindowExposeEvent(core::events::WindowExposeEvent* event) {
  UNUSED(event);
  if (stream_) {
    stream_->RefreshRequest();
  }
}

void Player::HandleWindowCloseEvent(core::events::WindowCloseEvent* event) {
  UNUSED(event);
  Quit();
}

void Player::HandleQuitEvent(core::events::QuitEvent* event) {
  UNUSED(event);
  Quit();
}

void Player::HandleClientConnectedEvent(core::events::ClientConnectedEvent* event) {
  UNUSED(event);
  SwitchToAuthorizeMode();
}

void Player::HandleClientDisconnectedEvent(core::events::ClientDisconnectedEvent* event) {
  UNUSED(event);
  if (current_state_ == INIT_STATE) {
    SwitchToDisconnectMode();
  }
}

void Player::HandleClientAuthorizedEvent(core::events::ClientAuthorizedEvent* event) {
  UNUSED(event);

  controller_->RequestServerInfo();
}

void Player::HandleClientUnAuthorizedEvent(core::events::ClientUnAuthorizedEvent* event) {
  UNUSED(event);
  if (current_state_ == INIT_STATE) {
    SwitchToDisconnectMode();
  }
}

void Player::HandleClientConfigChangeEvent(core::events::ClientConfigChangeEvent* event) {
  UNUSED(event);
}

void Player::HandleReceiveChannelsEvent(core::events::ReceiveChannelsEvent* event) {
  ChannelsInfo chan = event->info();
  // prepare cache folders
  ChannelsInfo::channels_t channels = chan.GetChannels();
  const std::string cache_dir = common::file_system::make_path(app_directory_absolute_path_, CACHE_FOLDER_NAME);
  bool is_exist_cache_root = common::file_system::is_directory_exist(cache_dir);
  if (!is_exist_cache_root) {
    common::ErrnoError err = common::file_system::create_directory(cache_dir, true);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    } else {
      is_exist_cache_root = true;
    }
  }

  for (const ChannelInfo& ch : channels) {
    PlaylistEntry entry = PlaylistEntry(cache_dir, ch);
    play_list_.push_back(entry);

    if (is_exist_cache_root) {  // prepare cache folders for channels
      const std::string channel_dir = entry.GetCacheDir();
      bool is_cache_channel_dir_exist = common::file_system::is_directory_exist(channel_dir);
      if (!is_cache_channel_dir_exist) {
        common::ErrnoError err = common::file_system::create_directory(channel_dir, true);
        if (err && err->IsError()) {
          DEBUG_MSG_ERROR(err);
        } else {
          is_cache_channel_dir_exist = true;
        }
      }

      if (!is_cache_channel_dir_exist) {
        continue;
      }

      EpgInfo epg = ch.GetEpg();
      common::uri::Uri uri = epg.GetIconUrl();
      bool is_unknown_icon = EpgInfo::IsUnknownIconUrl(uri);
      if (is_unknown_icon) {
        continue;
      }

      auto load_image_cb = [entry, uri, channel_dir]() {
        const std::string channel_icon_path = entry.GetIconPath();
        if (!common::file_system::is_file_exist(channel_icon_path)) {  // if not exist trying to download
          common::buffer_t buff;
          bool is_file_downloaded = DownloadFileToBuffer(uri, &buff);
          if (!is_file_downloaded) {
            return;
          }

          const uint32_t fl = common::file_system::File::FLAG_CREATE | common::file_system::File::FLAG_WRITE |
                              common::file_system::File::FLAG_OPEN_BINARY;
          common::file_system::File channel_icon_file;
          common::ErrnoError err = channel_icon_file.Open(channel_icon_path, fl);
          if (err && err->IsError()) {
            DEBUG_MSG_ERROR(err);
            return;
          }

          size_t writed;
          err = channel_icon_file.Write(buff, &writed);
          if (err && err->IsError()) {
            DEBUG_MSG_ERROR(err);
            err = channel_icon_file.Close();
            return;
          }

          err = channel_icon_file.Close();
        }
      };
      controller_->ExecInLoopThread(load_image_cb);
    }
  }

  SwitchToPlayingMode();
}

void Player::HandleBandwidthEstimationEvent(core::events::BandwidthEstimationEvent* event) {
  core::events::BandwidtInfo band_inf = event->info();
  if (band_inf.host_type == MAIN_SERVER) {
    controller_->RequestChannels();
  }
}

std::string Player::GetCurrentUrlName() const {
  PlaylistEntry url;
  if (GetCurrentUrl(&url)) {
    ChannelInfo ch = url.GetChannelInfo();
    return ch.GetName();
  }

  return "Unknown";
}

void Player::UpdateDisplayInterval(AVRational fps) {
  if (fps.num == 0) {
    fps = min_fps;
  }

  double frames_per_sec = fps.den / static_cast<double>(fps.num);
  update_video_timer_interval_msec_ = static_cast<uint32_t>(frames_per_sec * 1000 * 0.5);
}

bool Player::GetCurrentUrl(PlaylistEntry* url) const {
  if (!url || play_list_.empty()) {
    return false;
  }

  *url = play_list_[current_stream_pos_];
  return true;
}

void Player::sdl_audio_callback(void* user_data, uint8_t* stream, int len) {
  Player* player = static_cast<Player*>(user_data);
  core::VideoState* st = player->stream_;
  if (!player->muted_ && st && st->IsStreamReady()) {
    st->UpdateAudioBuffer(stream, len, player->options_.audio_volume);
  } else {
    memset(stream, 0, len);
  }
}

void Player::UpdateVolume(int step) {
  options_.audio_volume = stable_value_in_range(options_.audio_volume + step, 0, 100);
  show_volume_ = true;
  core::msec_t cur_time = core::GetCurrentMsec();
  volume_last_shown_ = cur_time;
}

void Player::Quit() {
  fApp->Exit(EXIT_SUCCESS);
}

void Player::SwitchToPlayingMode() {
  if (play_list_.empty()) {
    return;
  }

  size_t pos = current_stream_pos_;
  for (size_t i = 0; i < play_list_.size() && options_.last_showed_channel_id != invalid_stream_id; ++i) {
    PlaylistEntry ent = play_list_[i];
    ChannelInfo ch = ent.GetChannelInfo();
    if (ch.GetId() == options_.last_showed_channel_id) {
      pos = i;
      break;
    }
  }

  stream_ = CreateStreamPos(pos);
  if (stream_) {
    return;
  }

  common::Error err = common::make_error_value("Failed to create stream", common::Value::E_ERROR);
  SwitchToChannelErrorMode(err);
}

void Player::SwitchToChannelErrorMode(common::Error err) {
  std::string url_str = GetCurrentUrlName();
  std::string error_str = common::MemSPrintf("%s (%s)", url_str, err->Description());
  RUNTIME_LOG(err->GetLevel()) << error_str;
  InitWindow(error_str, FAILED_STATE);
}

void Player::SwitchToConnectMode() {
  InitWindow("Connecting...", INIT_STATE);
}

void Player::SwitchToDisconnectMode() {
  InitWindow("Disconnected", INIT_STATE);
}

void Player::SwitchToAuthorizeMode() {
  InitWindow("Authorize...", INIT_STATE);
}

void Player::SwitchToUnAuthorizeMode() {
  InitWindow("UnAuthorize...", INIT_STATE);
}

void Player::DrawDisplay() {
  if (current_state_ == PLAYING_STATE) {
    DrawPlayingStatus();
  } else if (current_state_ == INIT_STATE) {
    DrawInitStatus();
  } else if (current_state_ == FAILED_STATE) {
    DrawFailedStatus();
  } else {
    NOTREACHED();
  };
  video_frames_handled_++;
}

void Player::DrawFailedStatus() {
  if (!renderer_) {
    return;
  }

  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
  SDL_RenderClear(renderer_);
  if (offline_channel_texture_) {
    SDL_Texture* img = offline_channel_texture_->GetTexture(renderer_);
    if (img) {
      SDL_RenderCopy(renderer_, img, NULL, NULL);
    }
  }
  DrawInfo();
  SDL_RenderPresent(renderer_);
}

void Player::DrawPlayingStatus() {
  CHECK(THREAD_MANAGER()->IsMainThread());
  core::frames::VideoFrame* frame = stream_->TryToGetVideoFrame();
  uint32_t frames_per_sec = 1000 / update_video_timer_interval_msec_;
  bool need_to_check_is_alive = video_frames_handled_ % (frames_per_sec * no_data_panic_sec) == 0;
  if (need_to_check_is_alive) {
    core::VideoState::stats_t stats = stream_->GetStatistic();
    core::clock64_t cl = stats->master_pts;
    if (!stream_->IsPaused() && (last_pts_checkpoint_ == cl && cl != core::invalid_clock())) {
      common::Error err = common::make_error_value("No input data!", common::Value::E_ERROR);
      stream_->Abort();
      destroy(&stream_);
      SwitchToChannelErrorMode(err);
      last_pts_checkpoint_ = core::invalid_clock();
      return;
    }
    last_pts_checkpoint_ = cl;
  }

  if (!frame || !render_texture_ || !renderer_) {
    return;
  }

  int format = frame->format;
  int width = frame->width;
  int height = frame->height;

  Uint32 sdl_format;
  if (format == AV_PIX_FMT_YUV420P) {
    sdl_format = SDL_PIXELFORMAT_YV12;
  } else {
    sdl_format = SDL_PIXELFORMAT_ARGB8888;
  }

  SDL_Texture* texture = render_texture_->GetTexture(renderer_, width, height, sdl_format);
  if (!texture) {
    /* SDL allocates a buffer smaller than requested if the video
     * overlay hardware is unable to support the requested size. */

    ERROR_LOG() << "Error: the video system does not support an image\n"
                   "size of "
                << width << "x" << height << " pixels. Try using -lowres or -vf \"scale=w:h\"\n"
                                             "to reduce the image size.";
    return;
  }

  common::Error err = UploadTexture(texture, frame->frame);
  if (err && err->IsError()) {
    DEBUG_MSG_ERROR(err);
    return;
  }

  bool flip_v = frame->frame->linesize[0] < 0;

  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
  SDL_RenderClear(renderer_);

  SDL_Rect rect = core::calculate_display_rect(xleft_, ytop_, window_size_.width, window_size_.height, frame->width,
                                               frame->height, frame->sar);
  SDL_RenderCopyEx(renderer_, texture, NULL, &rect, 0, NULL, flip_v ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE);

  DrawInfo();
  SDL_RenderPresent(renderer_);
}  // namespace client

void Player::DrawInitStatus() {
  if (!renderer_) {
    return;
  }

  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
  SDL_RenderClear(renderer_);
  if (connection_error_texture_) {
    SDL_Texture* img = connection_error_texture_->GetTexture(renderer_);
    if (img) {
      SDL_RenderCopy(renderer_, img, NULL, NULL);
    }
  }
  DrawInfo();
  SDL_RenderPresent(renderer_);
}

void Player::DrawInfo() {
  DrawStatistic();
  DrawFooter();
  DrawVolume();
}

SDL_Rect Player::GetStatisticRect() const {
  const SDL_Rect display_rect = GetDisplayRect();
  return {display_rect.x, display_rect.y, display_rect.w / 3, display_rect.h};
}

SDL_Rect Player::GetFooterRect() const {
  const SDL_Rect display_rect = GetDrawRect();
  return {display_rect.x, display_rect.h - footer_height - volume_height - space_height + display_rect.y,
          display_rect.w, footer_height};
}

SDL_Rect Player::GetVolumeRect() const {
  const SDL_Rect display_rect = GetDrawRect();
  return {display_rect.x, display_rect.h - volume_height + display_rect.y, display_rect.w, volume_height};
}

SDL_Rect Player::GetDrawRect() const {
  const SDL_Rect dr = GetDisplayRect();
  return {dr.x + x_start, dr.y + y_start, dr.w - x_start * 2, dr.h - y_start * 2};
}

SDL_Rect Player::GetDisplayRect() const {
  const core::Size display_size = window_size_;
  return {0, 0, display_size.width, display_size.height};
}

void Player::DrawStatistic() {
  if (!show_statstic_ || !font_ || !renderer_) {
    return;
  }

  core::VideoState::stats_t stats = common::make_shared<core::Stats>();
  if (stream_) {
    stats = stream_->GetStatistic();
  }

  const SDL_Rect statistic_rect = GetStatisticRect();
  const bool is_unknown = stats->fmt == core::UNKNOWN_STREAM;

  std::string fmt_text = (is_unknown ? "N/A" : core::ConvertStreamFormatToString(stats->fmt));
  std::string hwaccel_text = (is_unknown ? "N/A" : common::ConvertToString(stats->active_hwaccel));
  std::transform(hwaccel_text.begin(), hwaccel_text.end(), hwaccel_text.begin(), ::toupper);
  double pts = stats->master_clock / 1000.0;
  std::string pts_text = (is_unknown ? "N/A" : common::ConvertToString(pts, 3));
  std::string fps_text = (is_unknown ? "N/A" : common::ConvertToString(stats->GetFps()));
  core::clock64_t diff = stats->GetDiffStreams();
  std::string diff_text = (is_unknown ? "N/A" : common::ConvertToString(diff));
  std::string fd_text = (stats->fmt & core::HAVE_VIDEO_STREAM
                             ? common::MemSPrintf("%d/%d", stats->frame_drops_early, stats->frame_drops_late)
                             : "N/A");
  std::string vbitrate_text =
      (stats->fmt & core::HAVE_VIDEO_STREAM ? common::ConvertToString(stats->video_bandwidth * 8 / 1024) : "N/A");
  std::string abitrate_text =
      (stats->fmt & core::HAVE_AUDIO_STREAM ? common::ConvertToString(stats->audio_bandwidth * 8 / 1024) : "N/A");
  std::string video_queue_text =
      (stats->fmt & core::HAVE_VIDEO_STREAM ? common::ConvertToString(stats->video_queue_size / 1024) : "N/A");
  std::string audio_queue_text =
      (stats->fmt & core::HAVE_AUDIO_STREAM ? common::ConvertToString(stats->audio_queue_size / 1024) : "N/A");

#define STATS_LINES_COUNT 10
  const std::string result_text = common::MemSPrintf(
      "FMT: %s\n"
      "HWACCEL: %s\n"
      "DIFF: %s msec\n"
      "PTS: %s\n"
      "FPS: %s\n"
      "FRAMEDROP: %s\n"
      "VBITRATE: %s kb/s\n"
      "ABITRATE: %s kb/s\n"
      "VQUEUE: %s KB\n"
      "AQUEUE: %s KB",
      fmt_text, hwaccel_text, diff_text, pts_text, fps_text, fd_text, vbitrate_text, abitrate_text, video_queue_text,
      audio_queue_text);

  int h = TTF_FontLineSkip(font_) * STATS_LINES_COUNT;
  if (h > statistic_rect.h) {
    h = statistic_rect.h;
  }

  SDL_Rect dst = {statistic_rect.x, statistic_rect.y, statistic_rect.w, h};
  SDL_SetRenderDrawColor(renderer_, 171, 217, 98, Uint8(SDL_ALPHA_OPAQUE * 0.5));
  SDL_RenderFillRect(renderer_, &dst);

  DrawWrappedTextInRect(result_text, text_color, dst);
}

void Player::DrawFooter() {
  if (!show_footer_ || !font_ || !renderer_) {
    return;
  }

  const SDL_Rect footer_rect = GetFooterRect();
  int padding_left = footer_rect.w / 4;
  SDL_Rect sdl_footer_rect = {footer_rect.x + padding_left, footer_rect.y, footer_rect.w - padding_left * 2,
                              footer_rect.h};
  if (current_state_ == INIT_STATE) {
    std::string footer_text = current_state_str_;
    SDL_SetRenderDrawColor(renderer_, 193, 66, 66, Uint8(SDL_ALPHA_OPAQUE * 0.5));
    SDL_RenderFillRect(renderer_, &sdl_footer_rect);
    DrawCenterTextInRect(footer_text, text_color, sdl_footer_rect);
  } else if (current_state_ == FAILED_STATE) {
    std::string footer_text = current_state_str_;
    SDL_SetRenderDrawColor(renderer_, 193, 66, 66, Uint8(SDL_ALPHA_OPAQUE * 0.5));
    SDL_RenderFillRect(renderer_, &sdl_footer_rect);
    DrawCenterTextInRect(footer_text, text_color, sdl_footer_rect);
  } else if (current_state_ == PLAYING_STATE) {
    PlaylistEntry entry;
    if (GetCurrentUrl(&entry)) {
      std::string decr = "N/A";
      ChannelInfo url = entry.GetChannelInfo();
      EpgInfo epg = url.GetEpg();
      ProgrammeInfo prog;
      if (epg.FindProgrammeByTime(common::time::current_mstime(), &prog)) {
        decr = prog.GetTitle();
      }

      SDL_SetRenderDrawColor(renderer_, 98, 118, 217, Uint8(SDL_ALPHA_OPAQUE * 0.5));
      SDL_RenderFillRect(renderer_, &sdl_footer_rect);

      std::string footer_text = common::MemSPrintf(
          " Title: %s\n"
          " Description: %s",
          url.GetName(), decr);
      int h = CalcHeightFontPlaceByRowCount(font_, 2);
      if (h > footer_rect.h) {
        h = footer_rect.h;
      }

      auto icon = entry.GetIcon();
      int shift = 0;
      if (icon) {
        SDL_Texture* img = icon->GetTexture(renderer_);
        if (img) {
          SDL_Rect icon_rect = {sdl_footer_rect.x, sdl_footer_rect.y, h, h};
          SDL_RenderCopy(renderer_, img, NULL, &icon_rect);
          shift = h;
        }
      }

      SDL_Rect text_rect = {sdl_footer_rect.x + shift, sdl_footer_rect.y, sdl_footer_rect.w - shift, h};
      DrawWrappedTextInRect(footer_text, text_color, text_rect);
    }
  } else {
    NOTREACHED();
  }
}

void Player::DrawVolume() {
  if (!show_volume_ || !font_ || !renderer_) {
    return;
  }

  int vol = options_.audio_volume;
  std::string vol_str = common::MemSPrintf("VOLUME: %d", vol);
  const SDL_Rect volume_rect = GetVolumeRect();
  int padding_left = volume_rect.w / 4;
  SDL_Rect sdl_volume_rect = {volume_rect.x + padding_left, volume_rect.y, volume_rect.w - padding_left * 2,
                              volume_rect.h};
  SDL_SetRenderDrawColor(renderer_, 171, 217, 98, Uint8(SDL_ALPHA_OPAQUE * 0.5));
  SDL_RenderFillRect(renderer_, &sdl_volume_rect);

  DrawCenterTextInRect(vol_str, text_color, sdl_volume_rect);
}

void Player::DrawCenterTextInRect(const std::string& text, SDL_Color text_color, SDL_Rect rect) {
  if (!renderer_ || !font_ || text.empty()) {
    DNOTREACHED();
    return;
  }

  SDL_Surface* text_surf = TTF_RenderText_Blended(font_, text.c_str(), text_color);
  SDL_Rect dst = GetCenterRect(rect, text_surf->w, text_surf->h);
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, text_surf);
  SDL_RenderCopy(renderer_, texture, NULL, &dst);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(text_surf);
}

void Player::DrawWrappedTextInRect(const std::string& text, SDL_Color text_color, SDL_Rect rect) {
  if (!renderer_ || !font_ || text.empty()) {
    DNOTREACHED();
    return;
  }

  SDL_Surface* text_surf = TTF_RenderText_Blended_Wrapped(font_, text.c_str(), text_color, rect.w);
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, text_surf);
  rect.w = text_surf->w;
  SDL_RenderCopy(renderer_, texture, NULL, &rect);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(text_surf);
}

void Player::InitWindow(const std::string& title, States status) {
  CalculateDispalySize();
  if (!window_) {
    if (!core::create_window(window_size_, options_.is_full_screen, title, &renderer_, &window_)) {
      return;
    }
  }

  SDL_SetWindowTitle(window_, title.c_str());
  current_state_ = status;
  current_state_str_ = title;
  StartShowFooter();
}

void Player::StartShowFooter() {
  show_footer_ = true;
  core::msec_t cur_time = core::GetCurrentMsec();
  footer_last_shown_ = cur_time;
}

void Player::CalculateDispalySize() {
  if (window_size_.IsValid()) {
    return;
  }

  if (options_.screen_size.IsValid()) {
    window_size_ = options_.screen_size;
  } else {
    window_size_ = options_.default_size;
  }
}

void Player::ToggleStatistic() {
  show_statstic_ = !show_statstic_;
}

void Player::ToggleMute() {
  bool muted = !muted_;
  SetMute(muted);
}

void Player::PauseStream() {
  if (stream_) {
    stream_->TogglePause();
  }
}

void Player::MoveToNextStream() {
  core::VideoState* st = stream_;
  if (st) {
    common::thread th([st]() {
      st->Abort();
      delete st;
    });
    th.detach();
  }
  stream_ = CreateNextStream();
  if (!stream_) {
    common::Error err = common::make_error_value("Failed to create stream", common::Value::E_ERROR);
    SwitchToChannelErrorMode(err);
  }
}

void Player::MoveToPreviousStream() {
  core::VideoState* st = stream_;
  if (st) {
    common::thread th([st]() {
      st->Abort();
      delete st;
    });
    th.detach();
  }
  stream_ = CreatePrevStream();
  if (!stream_) {
    common::Error err = common::make_error_value("Failed to create stream", common::Value::E_ERROR);
    SwitchToChannelErrorMode(err);
  }
}

core::VideoState* Player::CreateNextStream() {
  CHECK(THREAD_MANAGER()->IsMainThread());
  if (play_list_.empty()) {
    return nullptr;
  }

  size_t pos = GenerateNextPosition();
  core::VideoState* stream = CreateStreamPos(pos);
  return stream;
}

core::VideoState* Player::CreatePrevStream() {
  CHECK(THREAD_MANAGER()->IsMainThread());
  if (play_list_.empty()) {
    return nullptr;
  }

  size_t pos = GeneratePrevPosition();
  core::VideoState* stream = CreateStreamPos(pos);
  return stream;
}

core::VideoState* Player::CreateStreamPos(size_t pos) {
  CHECK(THREAD_MANAGER()->IsMainThread());
  current_stream_pos_ = pos;

  PlaylistEntry entr = play_list_[current_stream_pos_];
  if (!entr.GetIcon()) {  // try to upload image
    const std::string icon_path = entr.GetIconPath();
    const char* channel_icon_img_full_path_ptr = common::utils::c_strornull(icon_path);
    SDL_Surface* surface = IMG_Load(channel_icon_img_full_path_ptr);
    channel_icon_t shared_surface = common::make_shared<SurfaceSaver>(surface);
    play_list_[current_stream_pos_].SetIcon(shared_surface);
  }

  ChannelInfo url = entr.GetChannelInfo();
  stream_id sid = url.GetId();
  core::AppOptions copy = opt_;
  copy.enable_audio = url.IsEnableAudio();
  copy.enable_video = url.IsEnableVideo();
  core::VideoState* stream = new core::VideoState(sid, url.GetUrl(), copy, copt_, this);
  options_.last_showed_channel_id = sid;

  int res = stream->Exec();
  if (res == EXIT_FAILURE) {
    delete stream;
    return nullptr;
  }

  return stream;
}

size_t Player::GenerateNextPosition() const {
  if (current_stream_pos_ + 1 == play_list_.size()) {
    return 0;
  }

  return current_stream_pos_ + 1;
}

size_t Player::GeneratePrevPosition() const {
  if (current_stream_pos_ == 0) {
    return play_list_.size() - 1;
  }

  return current_stream_pos_ - 1;
}

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
