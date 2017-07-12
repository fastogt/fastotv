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

#include "client/simple_player.h"

#include <common/application/application.h>  // for fApp, Application
#include <common/threads/thread_manager.h>

#include <common/file_system.h>
#include <common/utils.h>
#include <common/convert2string.h>

#include "client/sdl_utils.h"
#include "client/av_sdl_utils.h"

#include "client/core/video_state.h"         // for VideoState
#include "client/core/frames/audio_frame.h"  // for AudioFrame
#include "client/core/frames/video_frame.h"  // for VideoFrame
#include "client/core/sdl_utils.h"

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

const SDL_Color SimplePlayer::text_color = {255, 255, 255, 0};
const AVRational SimplePlayer::min_fps = {25, 1};

SimplePlayer::SimplePlayer(const std::string& app_directory_absolute_path,
                           const PlayerOptions& options,
                           const core::AppOptions& opt,
                           const core::ComplexOptions& copt)
    : StreamHandler(),
      renderer_(NULL),
      font_(NULL),
      options_(options),
      opt_(opt),
      copt_(copt),
      app_directory_absolute_path_(app_directory_absolute_path),
      audio_params_(nullptr),
      audio_buff_size_(0),
      window_(NULL),
      show_cursor_(false),
      cursor_last_shown_(0),
      show_volume_(false),
      volume_last_shown_(0),
      last_mouse_left_click_(0),
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
}

void SimplePlayer::SetFullScreen(bool full_screen) {
  options_.is_full_screen = full_screen;
  SDL_SetWindowFullscreen(window_, full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  if (stream_) {
    stream_->RefreshRequest();
  }
}

void SimplePlayer::SetMute(bool mute) {
  muted_ = mute;
}

SimplePlayer::~SimplePlayer() {
  if (core::hw_device_ctx) {
    av_buffer_unref(&core::hw_device_ctx);
    core::hw_device_ctx = NULL;
  }
  fApp->UnSubscribe(this);
}

PlayerOptions SimplePlayer::GetOptions() const {
  return options_;
}

core::AppOptions SimplePlayer::GetStreamOptions() const {
  return opt_;
}

const std::string& SimplePlayer::GetAppDirectoryAbsolutePath() const {
  return app_directory_absolute_path_;
}

SimplePlayer::States SimplePlayer::GetCurrentState() const {
  return current_state_;
}

void SimplePlayer::HandleEvent(event_t* event) {
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

void SimplePlayer::HandleExceptionEvent(event_t* event, common::Error err) {
  UNUSED(event);
  UNUSED(err);
}

bool SimplePlayer::HandleRequestAudio(core::VideoState* stream,
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

void SimplePlayer::HanleAudioMix(uint8_t* audio_stream_ptr, const uint8_t* src, uint32_t len, int volume) {
  SDL_MixAudio(audio_stream_ptr, src, len, ConvertToSDLVolume(volume));
}

bool SimplePlayer::HandleRequestVideo(core::VideoState* stream,
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

void SimplePlayer::HandleRequestVideoEvent(core::events::RequestVideoEvent* event) {
  core::events::RequestVideoEvent* avent = static_cast<core::events::RequestVideoEvent*>(event);
  core::events::FrameInfo fr = avent->info();
  bool res = fr.stream_->RequestVideo(fr.width, fr.height, fr.av_pixel_format, fr.aspect_ratio);
  if (res) {
    return;
  }

  FreeStreamSafe();
  Quit();
}

void SimplePlayer::HandleQuitStreamEvent(core::events::QuitStreamEvent* event) {
  core::events::QuitStreamInfo inf = event->info();
  if (inf.stream_ && inf.stream_->IsAborted()) {
    return;
  }

  FreeStreamSafe();
  SwitchToChannelErrorMode(inf.error);
}

void SimplePlayer::HandlePreExecEvent(core::events::PreExecEvent* event) {
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

void SimplePlayer::HandlePostExecEvent(core::events::PostExecEvent* event) {
  core::events::PostExecInfo inf = event->info();
  if (inf.code == EXIT_SUCCESS) {
    FreeStreamSafe();
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

void SimplePlayer::HandleTimerEvent(core::events::TimerEvent* event) {
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

void SimplePlayer::HandleLircPressEvent(core::events::LircPressEvent* event) {
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

void SimplePlayer::HandleKeyPressEvent(core::events::KeyPressEvent* event) {
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
    default:
      break;
  }
}

void SimplePlayer::HandleMousePressEvent(core::events::MousePressEvent* event) {
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

void SimplePlayer::HandleMouseMoveEvent(core::events::MouseMoveEvent* event) {
  UNUSED(event);
  if (!show_cursor_) {
    fApp->ShowCursor();
    show_cursor_ = true;
  }
  core::msec_t cur_time = core::GetCurrentMsec();
  cursor_last_shown_ = cur_time;
}

void SimplePlayer::HandleWindowResizeEvent(core::events::WindowResizeEvent* event) {
  core::events::WindowResizeInfo inf = event->info();
  window_size_ = inf.size;
  if (stream_) {
    stream_->RefreshRequest();
  }
}

void SimplePlayer::HandleWindowExposeEvent(core::events::WindowExposeEvent* event) {
  UNUSED(event);
  if (stream_) {
    stream_->RefreshRequest();
  }
}

void SimplePlayer::HandleWindowCloseEvent(core::events::WindowCloseEvent* event) {
  UNUSED(event);
  Quit();
}

void SimplePlayer::HandleQuitEvent(core::events::QuitEvent* event) {
  UNUSED(event);
  Quit();
}

void SimplePlayer::FreeStreamSafe() {
  CHECK(THREAD_MANAGER()->IsMainThread());
  if (stream_) {
    stream_->Abort();
    destroy(&stream_);
  }
}

void SimplePlayer::UpdateDisplayInterval(AVRational fps) {
  if (fps.num == 0) {
    fps = min_fps;
  }

  double frames_per_sec = fps.den / static_cast<double>(fps.num);
  update_video_timer_interval_msec_ = static_cast<uint32_t>(frames_per_sec * 1000 * 0.5);
}

void SimplePlayer::sdl_audio_callback(void* user_data, uint8_t* stream, int len) {
  SimplePlayer* player = static_cast<SimplePlayer*>(user_data);
  core::VideoState* st = player->stream_;
  if (!player->muted_ && st && st->IsStreamReady()) {
    st->UpdateAudioBuffer(stream, len, player->options_.audio_volume);
  } else {
    memset(stream, 0, len);
  }
}

void SimplePlayer::UpdateVolume(int step) {
  options_.audio_volume = stable_value_in_range(options_.audio_volume + step, 0, 100);
  show_volume_ = true;
  core::msec_t cur_time = core::GetCurrentMsec();
  volume_last_shown_ = cur_time;
}

void SimplePlayer::Quit() {
  fApp->Exit(EXIT_SUCCESS);
}

void SimplePlayer::SwitchToChannelErrorMode(common::Error err) {
  std::string url_str = GetCurrentUrlName();
  std::string error_str = common::MemSPrintf("%s (%s)", url_str, err->Description());
  RUNTIME_LOG(err->GetLevel()) << error_str;
  InitWindow(error_str, FAILED_STATE);
}

void SimplePlayer::DrawDisplay() {
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

void SimplePlayer::DrawFailedStatus() {
  if (!renderer_) {
    return;
  }

  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
  SDL_RenderClear(renderer_);
  DrawInfo();
  SDL_RenderPresent(renderer_);
}

void SimplePlayer::DrawPlayingStatus() {
  CHECK(THREAD_MANAGER()->IsMainThread());
  core::frames::VideoFrame* frame = stream_->TryToGetVideoFrame();
  uint32_t frames_per_sec = 1000 / update_video_timer_interval_msec_;
  bool need_to_check_is_alive = video_frames_handled_ % (frames_per_sec * no_data_panic_sec) == 0;
  if (need_to_check_is_alive) {
    core::VideoState::stats_t stats = stream_->GetStatistic();
    core::clock64_t cl = stats->master_pts;
    if (!stream_->IsPaused() && (last_pts_checkpoint_ == cl && cl != core::invalid_clock())) {
      common::Error err = common::make_error_value("No input data!", common::Value::E_ERROR);
      FreeStreamSafe();
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

void SimplePlayer::DrawInitStatus() {
  if (!renderer_) {
    return;
  }

  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
  SDL_RenderClear(renderer_);
  DrawInfo();
  SDL_RenderPresent(renderer_);
}

void SimplePlayer::DrawInfo() {
  DrawStatistic();
  DrawVolume();
}

SDL_Rect SimplePlayer::GetStatisticRect() const {
  const SDL_Rect display_rect = GetDisplayRect();
  return {display_rect.x, display_rect.y, display_rect.w / 3, display_rect.h};
}

SDL_Rect SimplePlayer::GetVolumeRect() const {
  const SDL_Rect display_rect = GetDrawRect();
  return {display_rect.x, display_rect.h - volume_height + display_rect.y, display_rect.w, volume_height};
}

SDL_Rect SimplePlayer::GetDrawRect() const {
  const SDL_Rect dr = GetDisplayRect();
  return {dr.x + x_start, dr.y + y_start, dr.w - x_start * 2, dr.h - y_start * 2};
}

SDL_Rect SimplePlayer::GetDisplayRect() const {
  const core::Size display_size = window_size_;
  return {0, 0, display_size.width, display_size.height};
}

void SimplePlayer::DrawStatistic() {
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

void SimplePlayer::DrawVolume() {
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

void SimplePlayer::DrawCenterTextInRect(const std::string& text, SDL_Color text_color, SDL_Rect rect) {
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

void SimplePlayer::DrawWrappedTextInRect(const std::string& text, SDL_Color text_color, SDL_Rect rect) {
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

void SimplePlayer::InitWindow(const std::string& title, States status) {
  CalculateDispalySize();
  if (!window_) {
    if (!core::create_window(window_size_, options_.is_full_screen, title, &renderer_, &window_)) {
      return;
    }
  }

  SDL_SetWindowTitle(window_, title.c_str());
  current_state_ = status;
}

void SimplePlayer::CalculateDispalySize() {
  if (window_size_.IsValid()) {
    return;
  }

  if (options_.screen_size.IsValid()) {
    window_size_ = options_.screen_size;
  } else {
    window_size_ = options_.default_size;
  }
}

void SimplePlayer::ToggleStatistic() {
  show_statstic_ = !show_statstic_;
}

void SimplePlayer::ToggleMute() {
  bool muted = !muted_;
  SetMute(muted);
}

void SimplePlayer::PauseStream() {
  if (stream_) {
    stream_->TogglePause();
  }
}

void SimplePlayer::SetStream(core::VideoState* stream) {
  FreeStreamSafe();
  stream_ = stream;
  if (!stream_) {
    common::Error err = common::make_error_value("Failed to create stream", common::Value::E_ERROR);
    SwitchToChannelErrorMode(err);
  }
}

core::VideoState* SimplePlayer::CreateStream(stream_id sid, const common::uri::Uri& uri, core::AppOptions opt) {
  core::VideoState* stream = new core::VideoState(sid, uri, opt, copt_, this);
  options_.last_showed_channel_id = sid;
  int res = stream->Exec();
  if (res == EXIT_FAILURE) {
    delete stream;
    return nullptr;
  }

  return stream;
}

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
