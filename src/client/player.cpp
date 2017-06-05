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

#include <SDL2/SDL_audio.h>    // for SDL_CloseAudio, SDL_MIX_...
#include <SDL2/SDL_hints.h>    // for SDL_SetHint, SDL_HINT_RE...
#include <SDL2/SDL_pixels.h>   // for SDL_Color, ::SDL_PIXELFO...
#include <SDL2/SDL_rect.h>     // for SDL_Rect
#include <SDL2/SDL_surface.h>  // for SDL_Surface, SDL_FreeSur...

#include <common/application/application.h>  // for fApp, Application
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
#include "client/core/utils.h"            // for audio_open, calculate_di...
#include "client/core/video_frame.h"      // for VideoFrame
#include "client/core/video_state.h"      // for VideoState
#include "client/ioservice.h"             // for IoService
#include "client/sdl_utils.h"             // for IMG_LoadPNG, TextureSaver

#include "url.h"  // for Url

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

#undef ERROR

namespace fasto {
namespace fastotv {
namespace client {

namespace {

int ConvertToSDLVolume(int val) {
  val = av_clip(val, 0, 100);
  return av_clip(SDL_MIX_MAXVOLUME * val / 100, 0, SDL_MIX_MAXVOLUME);
}

bool CreateWindowFunc(Size window_size,
                      bool is_full_screen,
                      const std::string& title,
                      SDL_Renderer** renderer,
                      SDL_Window** window) {
  if (!renderer || !window || !window_size.IsValid()) {  // invalid input
    return false;
  }

  Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
  if (is_full_screen) {
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
  }
  SDL_Window* lwindow = SDL_CreateWindow(NULL, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_size.width,
                                         window_size.height, flags);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  SDL_Renderer* lrenderer = NULL;
  if (lwindow) {
    SDL_RendererInfo info;
    lrenderer = SDL_CreateRenderer(lwindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (lrenderer) {
      if (!SDL_GetRendererInfo(lrenderer, &info)) {
        DEBUG_LOG() << "Initialized " << info.name << " renderer.";
      }
    } else {
      WARNING_LOG() << "Failed to initialize a hardware accelerated renderer: " << SDL_GetError();
      lrenderer = SDL_CreateRenderer(lwindow, -1, 0);
    }
  }

  if (!lwindow || !lrenderer) {
    ERROR_LOG() << "SDL: could not set video mode - exiting";
    if (lrenderer) {
      SDL_DestroyRenderer(lrenderer);
    }
    if (lwindow) {
      SDL_DestroyWindow(lwindow);
    }
    return false;
  }

  SDL_SetWindowSize(lwindow, window_size.width, window_size.height);
  SDL_SetWindowTitle(lwindow, title.c_str());

  *window = lwindow;
  *renderer = lrenderer;
  return true;
}
}  // namespace

Player::Player(const PlayerOptions& options, const core::AppOptions& opt, const core::ComplexOptions& copt)
    : options_(options),
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
      curent_stream_pos_(0),
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
      muted_(false) {
  // stable options
  if (options_.audio_volume < 0) {
    WARNING_LOG() << "-volume=" << options_.audio_volume << " < 0, setting to 0";
  }
  if (options_.audio_volume > 100) {
    WARNING_LOG() << "-volume=" << options_.audio_volume << " > 100, setting to 100";
  }
  options_.audio_volume = av_clip(options_.audio_volume, 0, 100);

  fApp->Subscribe(this, core::events::PostExecEvent::EventType);
  fApp->Subscribe(this, core::events::PreExecEvent::EventType);
  fApp->Subscribe(this, core::events::TimerEvent::EventType);

  fApp->Subscribe(this, core::events::AllocFrameEvent::EventType);
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

void Player::Mute() {
  bool muted = !muted_;
  SetMute(muted);
}

Player::~Player() {
  if (core::hw_device_ctx) {
    av_buffer_unref(&core::hw_device_ctx);
    core::hw_device_ctx = NULL;
  }
  destroy(&controller_);
  fApp->UnSubscribe(this);
}

void Player::HandleEvent(event_t* event) {
  if (event->GetEventType() == core::events::PreExecEvent::EventType) {
    core::events::PreExecEvent* pre_event = static_cast<core::events::PreExecEvent*>(event);
    HandlePreExecEvent(pre_event);
  } else if (event->GetEventType() == core::events::PostExecEvent::EventType) {
    core::events::PostExecEvent* post_event = static_cast<core::events::PostExecEvent*>(event);
    HandlePostExecEvent(post_event);
  } else if (event->GetEventType() == core::events::AllocFrameEvent::EventType) {
    core::events::AllocFrameEvent* avent = static_cast<core::events::AllocFrameEvent*>(event);
    HandleAllocFrameEvent(avent);
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
  int ret = core::audio_open(this, wanted_channel_layout, wanted_nb_channels, wanted_sample_rate, &laudio_hw_params,
                             sdl_audio_callback);
  if (ret < 0) {
    return false;
  }

  SDL_PauseAudio(0);
  audio_params_ = new core::AudioParams(laudio_hw_params);
  audio_buff_size_ = ret;

  *audio_hw_params = *audio_params_;
  *audio_buff_size = audio_buff_size_;
  return true;
}

void Player::HanleAudioMix(uint8_t* audio_stream_ptr, const uint8_t* src, uint32_t len, int volume) {
  SDL_MixAudio(audio_stream_ptr, src, len, ConvertToSDLVolume(volume));
}

bool Player::HandleRealocFrame(core::VideoState* stream, core::VideoFrame* frame) {
  UNUSED(stream);

  Uint32 sdl_format;
  if (frame->format == AV_PIX_FMT_YUV420P) {
    sdl_format = SDL_PIXELFORMAT_YV12;
  } else {
    sdl_format = SDL_PIXELFORMAT_ARGB8888;
  }

  if (ReallocTexture(&frame->bmp, sdl_format, frame->width, frame->height, SDL_BLENDMODE_NONE, false) < 0) {
    /* SDL allocates a buffer smaller than requested if the video
     * overlay hardware is unable to support the requested size. */

    ERROR_LOG() << "Error: the video system does not support an image\n"
                   "size of "
                << frame->width << "x" << frame->height
                << " pixels. Try using -lowres or -vf \"scale=w:h\"\n"
                   "to reduce the image size.";
    return false;
  }

  return true;
}

void Player::HanleDisplayFrame(core::VideoState* stream, const core::VideoFrame* frame) {
  UNUSED(stream);
  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
  SDL_RenderClear(renderer_);

  SDL_Rect rect;
  core::calculate_display_rect(&rect, xleft_, ytop_, window_size_.width, window_size_.height, frame->width,
                               frame->height, frame->sar);
  SDL_RenderCopyEx(renderer_, frame->bmp, NULL, &rect, 0, NULL, frame->flip_v ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE);

  DrawInfo();
  SDL_RenderPresent(renderer_);
}

bool Player::HandleRequestVideo(core::VideoState* stream) {
  if (!stream) {  // invalid input
    return false;
  }

  InitWindow(GetCurrentUrlName(), PLAYING_STATE);
  return true;
}

void Player::HandleDefaultWindowSize(Size frame_size, AVRational sar) {
  SDL_Rect rect;
  core::calculate_display_rect(&rect, 0, 0, INT_MAX, frame_size.height, frame_size.width, frame_size.height, sar);
  options_.default_size.width = rect.w;
  options_.default_size.height = rect.h;
}

void Player::HandleAllocFrameEvent(core::events::AllocFrameEvent* event) {
  int res = event->info().stream_->HandleAllocPictureEvent();
  if (res == ERROR_RESULT_VALUE) {
    if (stream_) {
      stream_->Abort();
      destroy(&stream_);
    }
    Quit();
  }
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
    SDL_Surface* surface = NULL;
    common::Error err = IMG_LoadPNG(offline_channel_img_full_path_ptr, &surface);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    } else {
      offline_channel_texture_ = new TextureSaver(surface);
    }

    const std::string connection_error_img_full_path =
        common::file_system::make_path(absolute_source_dir, IMG_CONNECTION_ERROR_PATH_RELATIVE);
    const char* connection_error_img_full_path_ptr = common::utils::c_strornull(connection_error_img_full_path);
    SDL_Surface* surface2 = NULL;
    err = IMG_LoadPNG(connection_error_img_full_path_ptr, &surface2);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    } else {
      connection_error_texture_ = new TextureSaver(surface2);
    }

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
    destroy(&offline_channel_texture_);
    destroy(&connection_error_texture_);

    SDL_CloseAudio();
    destroy(&audio_params_);

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
      Mute();
      break;
    }
    default:
      break;
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
    case FASTO_KEY_p:
    case FASTO_KEY_SPACE:
      PauseStream();
      break;
    case FASTO_KEY_m: {
      Mute();
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
  play_list_ = chan;
  SwitchToPlayingMode();
}

void Player::HandleBandwidthEstimationEvent(core::events::BandwidthEstimationEvent* event) {
  core::events::BandwidtInfo band_inf = event->info();
  if (band_inf.host_type == MAIN_SERVER) {
    controller_->RequestChannels();
  }
}

std::string Player::GetCurrentUrlName() const {
  Url url;
  if (GetCurrentUrl(&url)) {
    return url.GetName();
  }

  return "Unknown";
}

bool Player::GetCurrentUrl(Url* url) const {
  auto channels = play_list_.GetChannels();
  if (!url || channels.empty()) {
    return false;
  }

  *url = channels[curent_stream_pos_];
  return true;
}

void Player::sdl_audio_callback(void* opaque, uint8_t* stream, int len) {
  Player* player = static_cast<Player*>(opaque);
  core::VideoState* st = player->stream_;
  if (!player->muted_ && st && st->IsStreamReady()) {
    st->UpdateAudioBuffer(stream, len, player->options_.audio_volume);
  } else {
    memset(stream, 0, len);
  }
}

void Player::UpdateVolume(int step) {
  options_.audio_volume = av_clip(options_.audio_volume + step, 0, 100);
  show_volume_ = true;
  core::msec_t cur_time = core::GetCurrentMsec();
  volume_last_shown_ = cur_time;
}

void Player::Quit() {
  fApp->Exit(EXIT_SUCCESS);
}

int Player::ReallocTexture(SDL_Texture** texture,
                           Uint32 new_format,
                           int new_width,
                           int new_height,
                           SDL_BlendMode blendmode,
                           bool init_texture) {
  Uint32 format;
  int access, w, h;
  if (SDL_QueryTexture(*texture, &format, &access, &w, &h) < 0 || new_width != w || new_height != h ||
      new_format != format) {
    SDL_DestroyTexture(*texture);
    *texture = NULL;

    SDL_Texture* ltexture = NULL;
    common::Error err = CreateTexture(renderer_, new_format, new_width, new_height, blendmode, init_texture, &ltexture);
    if (err && err->IsError()) {
      DNOTREACHED();
      return ERROR_RESULT_VALUE;
    }
    *texture = ltexture;
  }
  return SUCCESS_RESULT_VALUE;
}

void Player::SwitchToPlayingMode() {
  stream_ = CreateCurrentStream();
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
  InitWindow(error_str, PLAYING_STATE);
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
  } else {
    NOTREACHED();
  };
}

void Player::DrawPlayingStatus() {
  if (stream_) {
    stream_->TryRefreshVideo();
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

void Player::DrawInitStatus() {
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
  const Size display_size = window_size_;

  DrawChannelsInfo(display_size);
  DrawVideoInfo(display_size);
  DrawFooter();
  DrawVolume();
}

void Player::DrawChannelsInfo(Size display_size) {
  UNUSED(display_size);
}

void Player::DrawVideoInfo(Size display_size) {
  UNUSED(display_size);
}

Rect Player::GetFooterRect() const {
  const Size display_size = window_size_;
  int x = 0;
  int y = display_size.height - footer_height;
  int w = display_size.width;
  int h = footer_height;
  return Rect(x, y, w, h);
}

Rect Player::GetVolumeRect() const {
  const Size display_size = window_size_;
  int x = 0;
  int y = display_size.height - footer_height;
  int w = display_size.width;
  int h = footer_height;
  return Rect(x, y, w, h);
}

void Player::DrawFooter() {
  if (!show_footer_ || !font_) {
    return;
  }

  std::string footer_text;
  if (current_state_ == INIT_STATE) {
    footer_text = current_state_str_;
  } else if (current_state_ == PLAYING_STATE) {
    footer_text = current_state_str_;
  } else {
    NOTREACHED();
  }

  static const SDL_Color text_color = {255, 255, 255, 0};
  SDL_Surface* text = TTF_RenderText_Solid(font_, footer_text.c_str(), text_color);
  const Rect footer_rect = GetFooterRect();
  SDL_Rect dst = {footer_rect.w / 2 - text->w / 2, footer_rect.y + (footer_rect.h / 2 - text->h / 2), text->w, text->h};
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, text);
  SDL_RenderCopy(renderer_, texture, NULL, &dst);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(text);
}

void Player::DrawVolume() {
  if (!show_volume_ || !font_) {
    return;
  }

  int vol = options_.audio_volume;
  std::string vol_str = common::MemSPrintf("VOLUME: %d", vol);
  const Rect volume_rect = GetVolumeRect();

  static const SDL_Color text_color = {255, 255, 255, 0};
  SDL_Surface* text = TTF_RenderText_Solid(font_, vol_str.c_str(), text_color);
  SDL_Rect dst = {volume_rect.w / 2 - text->w / 2, volume_rect.y + (volume_rect.h / 2 - text->h / 2), text->w, text->h};
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, text);
  SDL_RenderCopy(renderer_, texture, NULL, &dst);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(text);
}

void Player::InitWindow(const std::string& title, States status) {
  CalculateDispalySize();
  if (!window_) {
    CreateWindowFunc(window_size_, options_.is_full_screen, title, &renderer_, &window_);
  } else {
    SDL_SetWindowTitle(window_, title.c_str());
  }

  current_state_ = status;
  current_state_str_ = title;
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

core::VideoState* Player::CreateCurrentStream() {
  if (play_list_.IsEmpty()) {
    return nullptr;
  }

  size_t pos = curent_stream_pos_;
  core::VideoState* stream = CreateStreamPos(pos);
  int res = stream->Exec();
  if (res == EXIT_FAILURE) {
    delete stream;
    return nullptr;
  }

  return stream;
}

core::VideoState* Player::CreateNextStream() {
  // check is executed in main thread?
  if (play_list_.IsEmpty()) {
    return nullptr;
  }

  size_t pos = GenerateNextPosition();
  core::VideoState* stream = CreateStreamPos(pos);
  int res = stream->Exec();
  if (res == EXIT_FAILURE) {
    delete stream;
    return nullptr;
  }

  return stream;
}

core::VideoState* Player::CreatePrevStream() {
  // check is executed in main thread?
  if (play_list_.IsEmpty()) {
    return nullptr;
  }

  size_t pos = GeneratePrevPosition();
  core::VideoState* stream = CreateStreamPos(pos);
  int res = stream->Exec();
  if (res == EXIT_FAILURE) {
    delete stream;
    return nullptr;
  }

  return stream;
}

core::VideoState* Player::CreateStreamPos(size_t pos) {
  CHECK(THREAD_MANAGER()->IsMainThread());
  curent_stream_pos_ = pos;
  auto channels = play_list_.GetChannels();
  Url url = channels[curent_stream_pos_];
  core::AppOptions copy = opt_;
  copy.disable_audio = !url.IsEnableAudio();
  copy.disable_video = !url.IsEnableVideo();
  core::VideoState* stream = new core::VideoState(url.GetId(), url.GetUrl(), copy, copt_, this);
  return stream;
}

size_t Player::GenerateNextPosition() const {
  if (curent_stream_pos_ + 1 == play_list_.Size()) {
    return 0;
  }

  return curent_stream_pos_ + 1;
}

size_t Player::GeneratePrevPosition() const {
  if (curent_stream_pos_ == 0) {
    return play_list_.Size() - 1;
  }

  return curent_stream_pos_ - 1;
}
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
