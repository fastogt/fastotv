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

#pragma once

#include <SDL2/SDL_ttf.h>  // for TTF_Font

#include <common/url.h>  // for Error

#include "client/player/player_options.h"
#include "client/player/stream_handler.h"

#include "client/player/core/app_options.h"    // for AppOptions, ComplexOp...
#include "client/player/core/events/events.h"  // for PostExecEvent, PreExe...

namespace common {
namespace threads {
template <typename RT>
class Thread;
}
}  // namespace common

namespace fasto {
namespace fastotv {
namespace client {

class TextureSaver;
namespace core {
struct AudioParams;
}  // namespace core

int CalcHeightFontPlaceByRowCount(const TTF_Font* font, int row);
bool CaclTextSize(const std::string& text, TTF_Font* font, int* width, int* height);
std::string DotText(std::string text, TTF_Font* font, int max_width);

class ISimplePlayer : public StreamHandler, public core::events::EventListener {
 public:
  enum {
    volume_height = 30,
    space_height = 10,
    space_width = 10,
    x_start = 10,
    y_start = 10,
    update_stats_timeout_msec = 1000,
    no_data_panic_sec = 60
  };
  static const SDL_Color text_color;
  static const AVRational min_fps;

  enum States { INIT_STATE, PLAYING_STATE, FAILED_STATE };

  void SetFullScreen(bool full_screen);
  void SetMute(bool mute);
  void UpdateVolume(int8_t step);
  void Quit();

  virtual ~ISimplePlayer();

  States GetCurrentState() const;

  virtual std::string GetCurrentUrlName() const = 0;

  PlayerOptions GetOptions() const;

  virtual void SetUrlLocation(stream_id sid,
                              const common::uri::Uri& uri,
                              core::AppOptions opt,
                              core::ComplexOptions copt);

 protected:
  explicit ISimplePlayer(const PlayerOptions& options);

  virtual void HandleEvent(event_t* event) override;
  virtual void HandleExceptionEvent(event_t* event, common::Error err) override;

  virtual common::Error HandleRequestAudio(core::VideoState* stream,
                                           int64_t wanted_channel_layout,
                                           int wanted_nb_channels,
                                           int wanted_sample_rate,
                                           core::AudioParams* audio_hw_params,
                                           int* audio_buff_size) override;
  virtual void HanleAudioMix(uint8_t* audio_stream_ptr, const uint8_t* src, uint32_t len, int volume) override;

  // should executed in gui thread
  virtual common::Error HandleRequestVideo(core::VideoState* stream,
                                           int width,
                                           int height,
                                           int av_pixel_format,
                                           AVRational aspect_ratio) override;

  virtual void HandlePreExecEvent(core::events::PreExecEvent* event);
  virtual void HandlePostExecEvent(core::events::PostExecEvent* event);

  virtual void HandleTimerEvent(core::events::TimerEvent* event);

  virtual void HandleRequestVideoEvent(core::events::RequestVideoEvent* event);
  virtual void HandleQuitStreamEvent(core::events::QuitStreamEvent* event);

  virtual void HandleKeyPressEvent(core::events::KeyPressEvent* event);

  virtual void HandleLircPressEvent(core::events::LircPressEvent* event);

  virtual void HandleWindowResizeEvent(core::events::WindowResizeEvent* event);
  virtual void HandleWindowExposeEvent(core::events::WindowExposeEvent* event);
  virtual void HandleWindowCloseEvent(core::events::WindowCloseEvent* event);

  virtual void HandleMousePressEvent(core::events::MousePressEvent* event);
  virtual void HandleMouseMoveEvent(core::events::MouseMoveEvent* event);

  virtual void HandleQuitEvent(core::events::QuitEvent* event);

  virtual void InitWindow(const std::string& title, States status);

  virtual void DrawDisplay();
  virtual void DrawPlayingStatus();
  virtual void DrawFailedStatus();
  virtual void DrawInitStatus();

  virtual void DrawInfo();  // statistic + volume

  virtual void DrawStatistic();
  virtual void DrawVolume();

  bool IsMouseVisible() const;

  core::VideoState* CreateStream(stream_id sid,
                                 const common::uri::Uri& uri,
                                 core::AppOptions opt,
                                 core::ComplexOptions copt);
  void SetStream(core::VideoState* stream);  // if stream == NULL => SwitchToChannelErrorMode

  SDL_Rect GetDrawRect() const;  // GetDisplayRect + with margins
  SDL_Rect GetDisplayRect() const;

  void DrawCenterTextInRect(const std::string& text, SDL_Color text_color, SDL_Rect rect);
  void DrawWrappedTextInRect(const std::string& text, SDL_Color text_color, SDL_Rect rect);

  SDL_Renderer* GetRenderer() const;
  TTF_Font* GetFont() const;

 private:
  void SwitchToChannelErrorMode(common::Error err);

  void FreeStreamSafe(bool fast_cleanup);

  void UpdateDisplayInterval(AVRational fps);

  /* prepare a new audio buffer */
  static void sdl_audio_callback(void* user_data, uint8_t* stream, int len);

  void CalculateDispalySize();

  // channel events
  void ToggleMute();
  void PauseStream();

  // player modes
  void ToggleShowStatistic();

  SDL_Rect GetStatisticRect() const;
  SDL_Rect GetVolumeRect() const;

  SDL_Renderer* renderer_;
  TTF_Font* font_;

  PlayerOptions options_;

  core::AudioParams* audio_params_;
  int audio_buff_size_;

  SDL_Window* window_;

  bool show_cursor_;
  core::msec_t cursor_last_shown_;

  bool show_volume_;
  core::msec_t volume_last_shown_;

  core::msec_t last_mouse_left_click_;

  std::shared_ptr<common::threads::Thread<int> > exec_tid_;
  core::VideoState* stream_;

  core::Size window_size_;
  const int xleft_;
  const int ytop_;

  States current_state_;

  bool muted_;
  bool show_statstic_;

  TextureSaver* render_texture_;

  uint32_t update_video_timer_interval_msec_;

  core::clock64_t last_pts_checkpoint_;
  size_t video_frames_handled_;
};

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
