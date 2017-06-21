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
#include <string>    // for string

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

#include "client/core/app_options.h"            // for AppOptions, ComplexOp...
#include "client/core/events/events.h"          // for PostExecEvent, PreExe...
#include "client/core/events/key_events.h"      // for KeyPressEvent
#include "client/core/events/lirc_events.h"     // for LircPressEvent
#include "client/core/events/mouse_events.h"    // for MouseMoveEvent, Mouse...
#include "client/core/events/network_events.h"  // for BandwidthEstimationEvent
#include "client/core/events/stream_events.h"   // for AllocFrameEvent, Quit...
#include "client/core/events/window_events.h"   // for WindowCloseEvent, Win...
#include "client/core/types.h"                  // for msec_t
#include "client/core/video_state_handler.h"    // for VideoStateHandler

namespace fasto {
namespace fastotv {
class ChannelInfo;
}
}  // namespace fasto
namespace fasto {
namespace fastotv {
namespace client {
class IoService;
}
}  // namespace fastotv
}  // namespace fasto
namespace fasto {
namespace fastotv {
namespace client {
class TextureSaver;
}
}  // namespace fastotv
}  // namespace fasto
namespace fasto {
namespace fastotv {
namespace client {
namespace core {
class VideoState;
}
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
namespace fasto {
namespace fastotv {
namespace client {
namespace core {
struct AudioParams;
}
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
namespace fasto {
namespace fastotv {
namespace client {
namespace core {
struct VideoFrame;
}
}  // namespace client
}  // namespace fastotv
}  // namespace fasto

namespace fasto {
namespace fastotv {
namespace client {

class Player : public core::VideoStateHandler {
 public:
  enum {
    footer_height = 60,
    volume_height = 30,
    space_height = 10,
    space_width = 10,
    x_start = 10,
    y_start = 10,
    update_stats_timeout_msec = 1000
  };
  static const SDL_Color text_color;

  enum States { INIT_STATE, PLAYING_STATE };
  Player(const std::string& app_directory_absolute_path,
         const PlayerOptions& options,
         const core::AppOptions& opt,
         const core::ComplexOptions& copt);
  void SetFullScreen(bool full_screen);
  void SetMute(bool mute);
  void Mute();
  void UpdateVolume(int step);
  void Quit();
  ~Player();

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

  virtual bool HandleReallocFrame(core::VideoState* stream, core::VideoFrame* frame) override;
  virtual void HanleDisplayFrame(core::VideoState* stream, const core::VideoFrame* frame) override;
  virtual bool HandleRequestVideo(core::VideoState* stream) override;
  virtual void HandleDefaultWindowSize(core::Size frame_size, AVRational sar) override;

  virtual void HandlePreExecEvent(core::events::PreExecEvent* event);
  virtual void HandlePostExecEvent(core::events::PostExecEvent* event);

  virtual void HandleTimerEvent(core::events::TimerEvent* event);

  virtual void HandleAllocFrameEvent(core::events::AllocFrameEvent* event);
  virtual void HandleQuitStreamEvent(core::events::QuitStreamEvent* event);

  virtual void HandleKeyPressEvent(core::events::KeyPressEvent* event);

  virtual void HandleLircPressEvent(core::events::LircPressEvent* event);

  virtual void HandleWindowResizeEvent(core::events::WindowResizeEvent* event);
  virtual void HandleWindowExposeEvent(core::events::WindowExposeEvent* event);
  virtual void HandleWindowCloseEvent(core::events::WindowCloseEvent* event);

  virtual void HandleMousePressEvent(core::events::MousePressEvent* event);
  virtual void HandleMouseMoveEvent(core::events::MouseMoveEvent* event);

  virtual void HandleQuitEvent(core::events::QuitEvent* event);

  virtual void HandleClientConnectedEvent(core::events::ClientConnectedEvent* event);
  virtual void HandleClientDisconnectedEvent(core::events::ClientDisconnectedEvent* event);
  virtual void HandleClientAuthorizedEvent(core::events::ClientAuthorizedEvent* event);
  virtual void HandleClientUnAuthorizedEvent(core::events::ClientUnAuthorizedEvent* event);
  virtual void HandleClientConfigChangeEvent(core::events::ClientConfigChangeEvent* event);
  virtual void HandleReceiveChannelsEvent(core::events::ReceiveChannelsEvent* event);
  virtual void HandleBandwidthEstimationEvent(core::events::BandwidthEstimationEvent* event);

 private:
  bool GetCurrentUrl(PlaylistEntry* url) const;
  std::string GetCurrentUrlName() const;  // return Unknown if not found

  /* prepare a new audio buffer */
  static void sdl_audio_callback(void* opaque, uint8_t* stream, int len);

  int ReallocTexture(SDL_Texture** texture,
                     Uint32 new_format,
                     int new_width,
                     int new_height,
                     SDL_BlendMode blendmode,
                     bool init_texture);

  void InitWindow(const std::string& title, States status);
  void StartShowFooter();
  void CalculateDispalySize();

  // channel evrnts
  void PauseStream();
  void MoveToNextStream();
  void MoveToPreviousStream();

  // player modes
  void ToggleStatistic();
  void SwitchToPlayingMode();
  void SwitchToChannelErrorMode(common::Error err);

  void SwitchToConnectMode();
  void SwitchToDisconnectMode();

  void SwitchToAuthorizeMode();
  void SwitchToUnAuthorizeMode();

  void DrawDisplay();
  void DrawPlayingStatus();
  void DrawInitStatus();

  void DrawInfo();
  void DrawStatistic();
  void DrawFooter();
  void DrawVolume();
  void DrawCenterTextInRect(const std::string& text, SDL_Color text_color, SDL_Rect rect);
  void DrawWrappedTextInRect(const std::string& text, SDL_Color text_color, SDL_Rect rect);

  SDL_Rect GetStatisticRect() const;
  SDL_Rect GetFooterRect() const;
  SDL_Rect GetVolumeRect() const;
  SDL_Rect GetDrawRect() const;  // GetDisplayRect + with margins
  SDL_Rect GetDisplayRect() const;

  core::VideoState* CreateCurrentStream();
  core::VideoState* CreateNextStream();
  core::VideoState* CreatePrevStream();
  core::VideoState* CreateStreamPos(size_t pos);

  size_t GenerateNextPosition() const;
  size_t GeneratePrevPosition() const;

  int CalcHeightFontPlaceByRowCount(int row) const;

  PlayerOptions options_;
  const core::AppOptions opt_;
  const core::ComplexOptions copt_;
  std::vector<PlaylistEntry> play_list_;

  core::AudioParams* audio_params_;
  int audio_buff_size_;

  SDL_Renderer* renderer_;
  SDL_Window* window_;

  bool show_cursor_;
  core::msec_t cursor_last_shown_;

  bool show_footer_;
  core::msec_t footer_last_shown_;

  bool show_volume_;
  core::msec_t volume_last_shown_;

  core::msec_t last_mouse_left_click_;
  size_t curent_stream_pos_;

  TextureSaver* offline_channel_texture_;
  TextureSaver* connection_error_texture_;

  TTF_Font* font_;
  core::VideoState* stream_;

  core::Size window_size_;
  const int xleft_;
  const int ytop_;

  IoService* controller_;
  States current_state_;
  std::string current_state_str_;

  bool muted_;
  bool show_statstic_;
  const std::string app_directory_absolute_path_;
};

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
