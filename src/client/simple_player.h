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

#include <stdint.h>  // for uint8_t, uint32_t

#include <string>  // for string

#include <SDL2/SDL_blendmode.h>  // for SDL_BlendMode
#include <SDL2/SDL_render.h>     // for SDL_Renderer, SDL_Tex...
#include <SDL2/SDL_stdinc.h>     // for Uint32
#include <SDL2/SDL_ttf.h>        // for TTF_Font
#include <SDL2/SDL_video.h>      // for SDL_Window'

#include <common/error.h>  // for Error

#include "auth_info.h"
#include "channels_info.h"  // for ChannelsInfo

#include "client/player_options.h"
#include "client/playlist_entry.h"

#include "client/core/events/events.h"          // for PostExecEvent, PreExe...
#include "client/core/events/key_events.h"      // for KeyPressEvent
#include "client/core/events/lirc_events.h"     // for LircPressEvent
#include "client/core/events/mouse_events.h"    // for MouseMoveEvent, Mouse...
#include "client/core/events/stream_events.h"   // for RequestVideoEvent, Quit...
#include "client/core/events/window_events.h"   // for WindowCloseEvent, Win...

#include "client/core/app_options.h"  // for AppOptions, ComplexOp...
#include "client/core/types.h"        // for msec_t

#include "client/stream_handler.h"

namespace fasto {
namespace fastotv {
class ChannelInfo;
namespace client {

class SurfaceSaver;
class TextureSaver;
namespace core {
class VideoState;
struct AudioParams;
struct VideoFrame;
}

int CalcHeightFontPlaceByRowCount(const TTF_Font* font, int row);

class SimplePlayer : public StreamHandler, public core::events::EventListener {
 public:
  enum {
    footer_height = 60,
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
  SimplePlayer(const std::string& app_directory_absolute_path,
               const PlayerOptions& options,
               const core::AppOptions& opt,
               const core::ComplexOptions& copt);

  void SetFullScreen(bool full_screen);
  void SetMute(bool mute);
  void UpdateVolume(int step);
  void Quit();

  virtual ~SimplePlayer();

  PlayerOptions GetOptions() const;
  core::AppOptions GetStreamOptions() const;

  const std::string& GetAppDirectoryAbsolutePath() const;
  States GetCurrentState() const;

  virtual std::string GetCurrentUrlName() const = 0;

 protected:
  virtual void HandleEvent(event_t* event) override;
  virtual void HandleExceptionEvent(event_t* event, common::Error err) override;

  virtual bool HandleRequestAudio(core::VideoState* stream,
                                  int64_t wanted_channel_layout,
                                  int wanted_nb_channels,
                                  int wanted_sample_rate,
                                  core::AudioParams* audio_hw_params,
                                  int* audio_buff_size) override;
  virtual void HanleAudioMix(uint8_t* audio_stream_ptr, const uint8_t* src, uint32_t len, int volume) override;

  // should executed in gui thread
  virtual bool HandleRequestVideo(core::VideoState* stream,
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

  void SwitchToChannelErrorMode(common::Error err);

  SDL_Renderer* renderer_;
  TTF_Font* font_;

  core::VideoState* CreateStream(stream_id sid, const common::uri::Uri& uri, core::AppOptions opt);
  void SetStream(core::VideoState* stream);  // if stream == NULL => SwitchToChannelErrorMode

  SDL_Rect GetDrawRect() const;  // GetDisplayRect + with margins

  void DrawCenterTextInRect(const std::string& text, SDL_Color text_color, SDL_Rect rect);
  void DrawWrappedTextInRect(const std::string& text, SDL_Color text_color, SDL_Rect rect);

 private:
  void FreeStreamSafe();

  void UpdateDisplayInterval(AVRational fps);

  /* prepare a new audio buffer */
  static void sdl_audio_callback(void* user_data, uint8_t* stream, int len);

  void CalculateDispalySize();

  // channel events
  void ToggleMute();
  void PauseStream();

  // player modes
  void ToggleStatistic();

  SDL_Rect GetStatisticRect() const;
  SDL_Rect GetVolumeRect() const;
  SDL_Rect GetDisplayRect() const;

  PlayerOptions options_;
  const core::AppOptions opt_;
  const core::ComplexOptions copt_;

  const std::string app_directory_absolute_path_;

  core::AudioParams* audio_params_;
  int audio_buff_size_;

  SDL_Window* window_;

  bool show_cursor_;
  core::msec_t cursor_last_shown_;

  bool show_volume_;
  core::msec_t volume_last_shown_;

  core::msec_t last_mouse_left_click_;

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
