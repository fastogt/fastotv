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

#include "client/isimple_player.h"

#include <thread>

#include <common/application/application.h>  // for fApp, Application
#include <common/threads/thread_manager.h>

#include <common/convert2string.h>
#include <common/file_system.h>
#include <common/utils.h>

#include "client/av_sdl_utils.h"
#include "client/sdl_utils.h"

#include "client/core/frames/audio_frame.h"  // for AudioFrame
#include "client/core/frames/video_frame.h"  // for VideoFrame
#include "client/core/sdl_utils.h"
#include "client/core/video_state.h"  // for VideoState

#include "client/core/application/sdl2_application.h"

/* Step size for volume control */
#define VOLUME_STEP 1

#define CURSOR_HIDE_DELAY_MSEC 1000  // 1 sec
#define VOLUME_HIDE_DELAY_MSEC 2000  // 2 sec

#define USER_FIELD "user"
#define URLS_FIELD "urls"

#define MAIN_FONT_PATH_RELATIVE "share/fonts/FreeSans.ttf"

namespace fasto {
namespace fastotv {
namespace client {

int CalcHeightFontPlaceByRowCount(const TTF_Font* font, int row) {
  if (!font) {
    return 0;
  }

  int font_height = TTF_FontLineSkip(font);
  return 2 << av_log2(font_height * row);
}

bool CaclTextSize(const std::string& text, TTF_Font* font, int* width, int* height) {
  const char* text_ptr = common::utils::c_strornull(text);
  if (!text_ptr || !font || !width || !height) {
    return false;
  }

  int res = TTF_SizeText(font, text_ptr, width, height);
  return res == 0;
}

std::string DotText(std::string text, TTF_Font* font, int max_width) {
  int needed_width, needed_height;
  if (CaclTextSize(text, font, &needed_width, &needed_height) && needed_width > max_width) {
    int char_width = needed_width / text.size();
    int diff = max_width / char_width;
    if (diff - 3 > 0) {
      text = text.substr(0, diff - 3) + "...";
    }
  }

  return text;
}

const SDL_Color ISimplePlayer::text_color = {255, 255, 255, 0};
const AVRational ISimplePlayer::min_fps = {25, 1};

ISimplePlayer::ISimplePlayer(const PlayerOptions& options)
    : StreamHandler(),
      renderer_(NULL),
      font_(NULL),
      options_(options),
      audio_params_(nullptr),
      audio_buff_size_(0),
      window_(NULL),
      show_cursor_(false),
      cursor_last_shown_(0),
      show_volume_(false),
      volume_last_shown_(0),
      last_mouse_left_click_(0),
      exec_tid_(),
      stream_(nullptr),
      window_size_(),
      xleft_(0),
      ytop_(0),
      current_state_(INIT_STATE),
      muted_(false),
      show_statstic_(false),
      render_texture_(NULL),
      update_video_timer_interval_msec_(0),
      last_pts_checkpoint_(core::invalid_clock()),
      video_frames_handled_(0) {
  UpdateDisplayInterval(min_fps);

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
}

void ISimplePlayer::SetFullScreen(bool full_screen) {
  options_.is_full_screen = full_screen;
  SDL_SetWindowFullscreen(window_, full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  if (stream_) {
    stream_->RefreshRequest();
  }
}

void ISimplePlayer::SetMute(bool mute) {
  muted_ = mute;
}

ISimplePlayer::~ISimplePlayer() {
  if (core::hw_device_ctx) {
    av_buffer_unref(&core::hw_device_ctx);
    core::hw_device_ctx = NULL;
  }
  fApp->UnSubscribe(this);
}

ISimplePlayer::States ISimplePlayer::GetCurrentState() const {
  return current_state_;
}

PlayerOptions ISimplePlayer::GetOptions() const {
  return options_;
}

void ISimplePlayer::SetUrlLocation(stream_id sid,
                                   const common::uri::Uri& uri,
                                   core::AppOptions opt,
                                   core::ComplexOptions copt) {
  core::VideoState* stream = CreateStream(sid, uri, opt, copt);
  SetStream(stream);
}

void ISimplePlayer::HandleEvent(event_t* event) {
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
  }
}

void ISimplePlayer::HandleExceptionEvent(event_t* event, common::Error err) {
  UNUSED(event);
  UNUSED(err);
}

common::Error ISimplePlayer::HandleRequestAudio(core::VideoState* stream,
                                                int64_t wanted_channel_layout,
                                                int wanted_nb_channels,
                                                int wanted_sample_rate,
                                                core::AudioParams* audio_hw_params,
                                                int* audio_buff_size) {
  UNUSED(stream);

  if (audio_params_) {
    *audio_hw_params = *audio_params_;
    *audio_buff_size = audio_buff_size_;
    return common::Error();
  }

  /* prepare audio output */
  core::AudioParams laudio_hw_params;
  int laudio_buff_size;
  if (!core::audio_open(this, wanted_channel_layout, wanted_nb_channels, wanted_sample_rate, sdl_audio_callback,
                        &laudio_hw_params, &laudio_buff_size)) {
    return common::make_error_value("Can't init audio system.", common::Value::E_ERROR);
  }

  SDL_PauseAudio(0);
  audio_params_ = new core::AudioParams(laudio_hw_params);
  audio_buff_size_ = laudio_buff_size;

  *audio_hw_params = *audio_params_;
  *audio_buff_size = audio_buff_size_;
  return common::Error();
}

void ISimplePlayer::HanleAudioMix(uint8_t* audio_stream_ptr, const uint8_t* src, uint32_t len, int volume) {
  SDL_MixAudio(audio_stream_ptr, src, len, ConvertToSDLVolume(volume));
}

common::Error ISimplePlayer::HandleRequestVideo(core::VideoState* stream,
                                                int width,
                                                int height,
                                                int av_pixel_format,
                                                AVRational aspect_ratio) {
  UNUSED(av_pixel_format);
  CHECK(THREAD_MANAGER()->IsMainThread());
  if (!stream) {  // invalid input
    return common::make_inval_error_value(common::Value::E_ERROR);
  }

  SDL_Rect rect = core::calculate_display_rect(0, 0, INT_MAX, height, width, height, aspect_ratio);
  options_.default_size.width = rect.w;
  options_.default_size.height = rect.h;

  InitWindow(GetCurrentUrlName(), PLAYING_STATE);

  AVRational frame_rate = stream->GetFrameRate();
  UpdateDisplayInterval(frame_rate);
  return common::Error();
}

void ISimplePlayer::HandleRequestVideoEvent(core::events::RequestVideoEvent* event) {
  core::events::RequestVideoEvent* avent = static_cast<core::events::RequestVideoEvent*>(event);
  core::events::FrameInfo fr = avent->info();
  common::Error err = fr.stream_->RequestVideo(fr.width, fr.height, fr.av_pixel_format, fr.aspect_ratio);
  if (err && err->IsError()) {
    SwitchToChannelErrorMode(err);
    Quit();
  }
}

void ISimplePlayer::HandleQuitStreamEvent(core::events::QuitStreamEvent* event) {
  core::events::QuitStreamInfo inf = event->info();
  if (inf.error && inf.error->IsError()) {
    SwitchToChannelErrorMode(inf.error);
  }
}

void ISimplePlayer::HandlePreExecEvent(core::events::PreExecEvent* event) {
  core::events::PreExecInfo inf = event->info();
  if (inf.code == EXIT_SUCCESS) {
    const std::string absolute_source_dir = common::file_system::absolute_path_from_relative(RELATIVE_SOURCE_DIR);
    render_texture_ = new TextureSaver;

    const std::string font_path = common::file_system::make_path(absolute_source_dir, MAIN_FONT_PATH_RELATIVE);
    const char* font_path_ptr = common::utils::c_strornull(font_path);
    font_ = TTF_OpenFont(font_path_ptr, 24);
    if (!font_) {
      WARNING_LOG() << "Couldn't open font file path: " << font_path;
    }
  }
}

void ISimplePlayer::HandlePostExecEvent(core::events::PostExecEvent* event) {
  core::events::PostExecInfo inf = event->info();
  if (inf.code == EXIT_SUCCESS) {
    FreeStreamSafe(false);
    if (font_) {
      TTF_CloseFont(font_);
      font_ = NULL;
    }

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

void ISimplePlayer::HandleTimerEvent(core::events::TimerEvent* event) {
  UNUSED(event);
  const core::msec_t cur_time = core::GetCurrentMsec();
  core::msec_t diff_currsor = cur_time - cursor_last_shown_;
  if (show_cursor_ && diff_currsor > CURSOR_HIDE_DELAY_MSEC) {
    fApp->HideCursor();
    show_cursor_ = false;
  }

  core::msec_t diff_volume = cur_time - volume_last_shown_;
  if (show_volume_ && diff_volume > VOLUME_HIDE_DELAY_MSEC) {
    show_volume_ = false;
  }
  DrawDisplay();
}

void ISimplePlayer::HandleLircPressEvent(core::events::LircPressEvent* event) {
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
    case LIRC_KEY_UP: {
      UpdateVolume(VOLUME_STEP);
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

void ISimplePlayer::HandleKeyPressEvent(core::events::KeyPressEvent* event) {
  if (options_.exit_on_keydown) {
    Quit();
    return;
  }

  const core::events::KeyPressInfo inf = event->info();
  const SDL_Scancode scan_code = inf.ks.scancode;
  const Uint32 modifier = inf.ks.mod;
  if (scan_code == SDL_SCANCODE_ESCAPE) {  // Quit
    Quit();
  } else if (scan_code == SDL_SCANCODE_F) {
    bool full_screen = !options_.is_full_screen;
    SetFullScreen(full_screen);
  } else if (scan_code == SDL_SCANCODE_F3) {
    ToggleShowStatistic();
  } else if (scan_code == SDL_SCANCODE_SPACE) {
    PauseStream();
  } else if (scan_code == SDL_SCANCODE_M) {
    ToggleMute();
  } else if (scan_code == SDL_SCANCODE_UP) {
    if (modifier & SDL_SCANCODE_LCTRL) {
      UpdateVolume(VOLUME_STEP);
    }
  } else if (scan_code == SDL_SCANCODE_DOWN) {
    if (modifier & SDL_SCANCODE_LCTRL) {
      UpdateVolume(-VOLUME_STEP);
    }
  } else if (scan_code == SDL_SCANCODE_S) {  // Step to next frame
    if (stream_) {
      stream_->StepToNextFrame();
    }
  } else if (scan_code == SDL_SCANCODE_A) {
    if (stream_) {
      stream_->StreamCycleChannel(AVMEDIA_TYPE_AUDIO);
    }
  } else if (scan_code == SDL_SCANCODE_V) {
    if (stream_) {
      stream_->StreamCycleChannel(AVMEDIA_TYPE_VIDEO);
    }
  } else if (scan_code == SDL_SCANCODE_C) {
    if (stream_) {
      stream_->StreamCycleChannel(AVMEDIA_TYPE_VIDEO);
      stream_->StreamCycleChannel(AVMEDIA_TYPE_AUDIO);
    }
  } else if (scan_code == SDL_SCANCODE_T) {
    // StreamCycleChannel(AVMEDIA_TYPE_SUBTITLE);
  } else if (scan_code == SDL_SCANCODE_W) {
  } else if (scan_code == SDL_SCANCODE_LEFT) {
    if (modifier & SDL_SCANCODE_LSHIFT) {
      if (stream_) {
        stream_->Seek(-10000);  // msec
      }
    } else if (modifier & SDL_SCANCODE_LALT) {
      if (stream_) {
        stream_->Seek(-60000);  // msec
      }
    } else if (modifier & SDL_SCANCODE_LCTRL) {
      if (stream_) {
        stream_->SeekPrevChunk();
      }
    }
  } else if (scan_code == SDL_SCANCODE_RIGHT) {
    if (modifier & SDL_SCANCODE_LSHIFT) {
      if (stream_) {
        stream_->Seek(10000);  // msec
      }
    } else if (modifier & SDL_SCANCODE_LALT) {
      if (stream_) {
        stream_->Seek(60000);  // msec
      }
    } else if (modifier & SDL_SCANCODE_LCTRL) {
      if (stream_) {
        stream_->SeekNextChunk();
      }
    }
  }
}

void ISimplePlayer::HandleMousePressEvent(core::events::MousePressEvent* event) {
  if (options_.exit_on_mousedown) {
    Quit();
    return;
  }

  core::msec_t cur_time = core::GetCurrentMsec();
  core::events::MousePressInfo inf = event->info();
  if (inf.mevent.button == SDL_BUTTON_LEFT) {
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

void ISimplePlayer::HandleMouseMoveEvent(core::events::MouseMoveEvent* event) {
  UNUSED(event);
  if (!show_cursor_) {
    fApp->ShowCursor();
    show_cursor_ = true;
  }
  core::msec_t cur_time = core::GetCurrentMsec();
  cursor_last_shown_ = cur_time;
}

void ISimplePlayer::HandleWindowResizeEvent(core::events::WindowResizeEvent* event) {
  core::events::WindowResizeInfo inf = event->info();
  window_size_ = inf.size;
  if (stream_) {
    stream_->RefreshRequest();
  }
}

void ISimplePlayer::HandleWindowExposeEvent(core::events::WindowExposeEvent* event) {
  UNUSED(event);
  if (stream_) {
    stream_->RefreshRequest();
  }
}

void ISimplePlayer::HandleWindowCloseEvent(core::events::WindowCloseEvent* event) {
  UNUSED(event);
  Quit();
}

void ISimplePlayer::HandleQuitEvent(core::events::QuitEvent* event) {
  UNUSED(event);
  Quit();
}

void ISimplePlayer::FreeStreamSafe(bool fast_cleanup) {
  CHECK(THREAD_MANAGER()->IsMainThread());
  if (!stream_) {
    return;
  }

  if (fast_cleanup) {
    core::VideoState* vs = stream_;
    auto tid = exec_tid_;

    exec_tid_.reset();
    stream_ = nullptr;

    vs->SetHandler(nullptr);
    std::thread out_cleanup([vs, tid]() {
      vs->Abort();
      tid->Join();
      delete vs;
    });
    out_cleanup.detach();
  } else {
    stream_->Abort();
    exec_tid_->Join();
    exec_tid_.reset();
    destroy(&stream_);
  }

  CHECK(!stream_);
  CHECK(!exec_tid_);
}

void ISimplePlayer::UpdateDisplayInterval(AVRational fps) {
  if (fps.num == 0) {
    fps = min_fps;
  }

  double frames_per_sec = fps.den / static_cast<double>(fps.num);
  update_video_timer_interval_msec_ = static_cast<uint32_t>(frames_per_sec * 1000);
  core::application::Sdl2Application* app = static_cast<core::application::Sdl2Application*>(fApp);
  app->SetDisplayUpdateTimeout(update_video_timer_interval_msec_);
}

void ISimplePlayer::sdl_audio_callback(void* user_data, uint8_t* stream, int len) {
  ISimplePlayer* player = static_cast<ISimplePlayer*>(user_data);
  core::VideoState* st = player->stream_;
  if (!player->muted_ && st && st->IsStreamReady()) {
    st->UpdateAudioBuffer(stream, len, player->options_.audio_volume);
  } else {
    memset(stream, 0, len);
  }
}

void ISimplePlayer::UpdateVolume(int8_t step) {
  options_.audio_volume = options_.audio_volume + step;
  show_volume_ = true;
  core::msec_t cur_time = core::GetCurrentMsec();
  volume_last_shown_ = cur_time;
}

void ISimplePlayer::Quit() {
  fApp->Exit(EXIT_SUCCESS);
}

void ISimplePlayer::SwitchToChannelErrorMode(common::Error err) {
  if (!err) {
    DNOTREACHED();
    return;
  }

  FreeStreamSafe(true);
  std::string url_str = GetCurrentUrlName();
  std::string err_descr = err->GetDescription();
  std::string error_str = common::MemSPrintf("%s (%s)", url_str, err_descr);
  RUNTIME_LOG(err->GetLevel()) << error_str;
  InitWindow(error_str, FAILED_STATE);
}

void ISimplePlayer::DrawDisplay() {
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

void ISimplePlayer::DrawFailedStatus() {
  if (!renderer_) {
    return;
  }

  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
  SDL_RenderClear(renderer_);
  DrawInfo();
  SDL_RenderPresent(renderer_);
}

void ISimplePlayer::DrawPlayingStatus() {
  CHECK(THREAD_MANAGER()->IsMainThread());
  core::frames::VideoFrame* frame = stream_->TryToGetVideoFrame();
  uint32_t frames_per_sec = 1000 / update_video_timer_interval_msec_;
  uint32_t mod = frames_per_sec * no_data_panic_sec;
  bool need_to_check_is_alive = video_frames_handled_ % mod == 0;
  if (need_to_check_is_alive) {
    INFO_LOG() << "No data checkpoint.";
    core::VideoState::stats_t stats = stream_->GetStatistic();
    core::clock64_t cl = stats->master_pts;
    if (!stream_->IsPaused() && (last_pts_checkpoint_ == cl && cl != core::invalid_clock())) {
      common::Error err = common::make_error_value("No input data!", common::Value::E_ERROR);
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
                << width << "x" << height
                << " pixels. Try using -lowres or -vf \"scale=w:h\"\n"
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

void ISimplePlayer::DrawInitStatus() {
  if (!renderer_) {
    return;
  }

  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
  SDL_RenderClear(renderer_);
  DrawInfo();
  SDL_RenderPresent(renderer_);
}

void ISimplePlayer::DrawInfo() {
  DrawStatistic();
  DrawVolume();
}

SDL_Rect ISimplePlayer::GetStatisticRect() const {
  const SDL_Rect display_rect = GetDisplayRect();
  return {display_rect.x, display_rect.y, display_rect.w / 3, display_rect.h};
}

SDL_Rect ISimplePlayer::GetVolumeRect() const {
  const SDL_Rect display_rect = GetDrawRect();
  return {display_rect.x, display_rect.h - volume_height + display_rect.y, display_rect.w, volume_height};
}

SDL_Rect ISimplePlayer::GetDrawRect() const {
  const SDL_Rect dr = GetDisplayRect();
  return {dr.x + x_start, dr.y + y_start, dr.w - x_start * 2, dr.h - y_start * 2};
}

SDL_Rect ISimplePlayer::GetDisplayRect() const {
  const core::Size display_size = window_size_;
  return {0, 0, display_size.width, display_size.height};
}

void ISimplePlayer::DrawStatistic() {
  if (!show_statstic_ || !font_ || !renderer_) {
    return;
  }

  core::VideoState::stats_t stats = std::make_shared<core::Stats>();
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

void ISimplePlayer::DrawVolume() {
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

bool ISimplePlayer::IsMouseVisible() const {
  return show_cursor_;
}

void ISimplePlayer::DrawCenterTextInRect(const std::string& text, SDL_Color text_color, SDL_Rect rect) {
  const char* text_ptr = common::utils::c_strornull(text);
  if (!renderer_ || !font_ || !text_ptr) {
    DNOTREACHED();
    return;
  }

  SDL_Surface* text_surf = TTF_RenderUTF8_Blended(font_, text_ptr, text_color);
  SDL_Rect dst = GetCenterRect(rect, text_surf->w, text_surf->h);
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, text_surf);
  SDL_RenderCopy(renderer_, texture, NULL, &dst);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(text_surf);
}

void ISimplePlayer::DrawWrappedTextInRect(const std::string& text, SDL_Color text_color, SDL_Rect rect) {
  const char* text_ptr = common::utils::c_strornull(text);
  if (!renderer_ || !font_ || !text_ptr) {
    DNOTREACHED();
    return;
  }

  SDL_Surface* text_surf = TTF_RenderUTF8_Blended_Wrapped(font_, text_ptr, text_color, rect.w);
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, text_surf);
  rect.w = text_surf->w;
  SDL_RenderCopy(renderer_, texture, NULL, &rect);
  SDL_DestroyTexture(texture);
  SDL_FreeSurface(text_surf);
}

SDL_Renderer* ISimplePlayer::GetRenderer() const {
  return renderer_;
}

TTF_Font* ISimplePlayer::GetFont() const {
  return font_;
}

void ISimplePlayer::InitWindow(const std::string& title, States status) {
  CalculateDispalySize();
  if (!window_) {
    if (!core::create_window(window_size_, options_.is_full_screen, title, &renderer_, &window_)) {
      return;
    }
  }

  SDL_SetWindowTitle(window_, title.c_str());
  current_state_ = status;
}

void ISimplePlayer::CalculateDispalySize() {
  if (window_size_.IsValid()) {
    return;
  }

  if (options_.screen_size.IsValid()) {
    window_size_ = options_.screen_size;
  } else {
    window_size_ = options_.default_size;
  }
}

void ISimplePlayer::ToggleShowStatistic() {
  show_statstic_ = !show_statstic_;
}

void ISimplePlayer::ToggleMute() {
  bool muted = !muted_;
  SetMute(muted);
}

void ISimplePlayer::PauseStream() {
  if (stream_) {
    stream_->TogglePause();
  }
}

void ISimplePlayer::SetStream(core::VideoState* stream) {
  FreeStreamSafe(true);
  stream_ = stream;

  if (!stream_) {
    common::Error err = common::make_error_value("Failed to create stream", common::Value::E_ERROR);
    SwitchToChannelErrorMode(err);
    return;
  }

  stream_->SetHandler(this);
  exec_tid_ = THREAD_MANAGER()->CreateThread(&core::VideoState::Exec, stream_);
  bool is_started = exec_tid_->Start();
  if (!is_started) {
    common::Error err = common::make_error_value("Failed to start stream", common::Value::E_ERROR);
    SwitchToChannelErrorMode(err);
  }
}

core::VideoState* ISimplePlayer::CreateStream(stream_id sid,
                                              const common::uri::Uri& uri,
                                              core::AppOptions opt,
                                              core::ComplexOptions copt) {
  core::VideoState* stream = new core::VideoState(sid, uri, opt, copt);
  options_.last_showed_channel_id = sid;
  return stream;
}

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
