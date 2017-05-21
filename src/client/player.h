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

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>

#include <common/smart_ptr.h>
#include <common/url.h>
#include <common/time.h>

#include "url.h"
#include "types.h"

#include "client/core/video_state_handler.h"
#include "client/core/app_options.h"
#include "client/core/events/events.h"

namespace fasto {
namespace fastotv {
namespace client {

class TextureSaver;

namespace core {
class VideoState;
}

class IoService;

struct PlayerOptions {
  enum { width = 640, height = 480, volume = 100 };
  PlayerOptions();

  bool exit_on_keydown;
  bool exit_on_mousedown;
  bool is_full_screen;

  Size default_size;
  Size screen_size;

  int audio_volume;  // Range: 0 - 100
};

class Player : public core::VideoStateHandler {
 public:
  enum { footer_height = 50 };
  enum States { INIT_STATE, PLAYING_STATE };
  Player(const PlayerOptions& options,
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
  virtual void HanleAudioMix(uint8_t* audio_stream_ptr,
                             const uint8_t* src,
                             uint32_t len,
                             int volume) override;

  virtual bool HandleRealocFrame(core::VideoState* stream, core::VideoFrame* frame) override;
  virtual void HanleDisplayFrame(core::VideoState* stream, const core::VideoFrame* frame) override;
  virtual bool HandleRequestVideo(core::VideoState* stream) override;
  virtual void HandleDefaultWindowSize(int width, int height, AVRational sar) override;

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

 private:
  bool GetCurrentUrl(Url* url) const;
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
  void CalculateDispalySize();

  // channel evrnts
  void PauseStream();
  void MoveToNextStream();
  void MoveToPreviousStream();

  // player modes
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
  void DrawChannelsInfo(Size display_size);
  void DrawVideoInfo(Size display_size);
  void DrawFooter();
  void DrawVolume();

  Rect GetFooterRect() const;
  Rect GetVolumeRect() const;

  core::VideoState* CreateCurrentStream();
  core::VideoState* CreateNextStream();
  core::VideoState* CreatePrevStream();
  core::VideoState* CreateStreamPos(size_t pos);

  size_t GenerateNextPosition() const;
  size_t GeneratePrevPosition() const;

  PlayerOptions options_;
  const core::AppOptions opt_;
  const core::ComplexOptions copt_;
  channels_t play_list_;

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

  Size window_size_;
  const int xleft_;
  const int ytop_;

  IoService* controller_;
  States current_state_;
  std::string current_state_str_;

  bool muted_;
};
}
}
}
