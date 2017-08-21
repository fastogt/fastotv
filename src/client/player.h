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

#include "client/isimple_player.h"
#include "client/playlist_entry.h"

#include "client/core/events/network_events.h"  // for BandwidthEstimationEvent

namespace fasto {
namespace fastotv {
namespace client {

class IoService;

struct ChannelDescription {
  size_t pos;
  std::string title;
  std::string description;
  channel_icon_t icon;
};

class Player : public ISimplePlayer {
 public:
  typedef ISimplePlayer base_class;
  typedef std::string keypad_sym_t;
  enum { footer_height = 60, keypad_height = 30, keypad_width = 60, min_key_pad_size = 0, max_keypad_size = 999 };
  Player(const std::string& app_directory_absolute_path,
         const PlayerOptions& options,
         const core::AppOptions& opt,
         const core::ComplexOptions& copt);

  ~Player();

  virtual std::string GetCurrentUrlName() const override;  // return Unknown if not found

  const std::string& GetAppDirectoryAbsolutePath() const;
  core::AppOptions GetStreamOptions() const;

 protected:
  virtual void HandleEvent(event_t* event) override;
  virtual void HandleExceptionEvent(event_t* event, common::Error err) override;

  virtual void HandlePreExecEvent(core::events::PreExecEvent* event) override;
  virtual void HandlePostExecEvent(core::events::PostExecEvent* event) override;

  virtual void HandleTimerEvent(core::events::TimerEvent* event) override;

  virtual void HandleBandwidthEstimationEvent(core::events::BandwidthEstimationEvent* event);
  virtual void HandleClientConnectedEvent(core::events::ClientConnectedEvent* event);
  virtual void HandleClientDisconnectedEvent(core::events::ClientDisconnectedEvent* event);
  virtual void HandleClientAuthorizedEvent(core::events::ClientAuthorizedEvent* event);
  virtual void HandleClientUnAuthorizedEvent(core::events::ClientUnAuthorizedEvent* event);
  virtual void HandleClientConfigChangeEvent(core::events::ClientConfigChangeEvent* event);
  virtual void HandleReceiveChannelsEvent(core::events::ReceiveChannelsEvent* event);

  virtual void HandleWindowResizeEvent(core::events::WindowResizeEvent* event) override;
  virtual void HandleWindowExposeEvent(core::events::WindowExposeEvent* event) override;

  virtual void HandleMousePressEvent(core::events::MousePressEvent* event) override;

  virtual void HandleKeyPressEvent(core::events::KeyPressEvent* event) override;
  virtual void HandleLircPressEvent(core::events::LircPressEvent* event) override;

  virtual void DrawInfo() override;
  virtual void DrawFailedStatus() override;
  virtual void DrawInitStatus() override;

  virtual void InitWindow(const std::string& title, States status) override;

 private:
  int GetMaxProgrammsLines() const;
  bool GetChannelDescription(size_t pos, ChannelDescription* descr) const;

  void HandleKeyPad(uint8_t key);
  void FinishKeyPadInput();
  void RemoveLastSymbolInKeypad();
  void CreateStreamPosAfterKeypad(size_t pos);
  void ResetKeyPad();
  SDL_Rect GetKeyPadRect() const;

  void DrawFooter();
  void DrawKeyPad();
  void DrawProgramsList();

  void StartShowFooter();
  SDL_Rect GetFooterRect() const;

  void ToggleShowProgramsList();
  SDL_Rect GetProgramsListRect() const;

  SDL_Rect GetHideButtonProgramsListRect() const;
  SDL_Rect GetShowButtonProgramsListRect() const;

  void MoveToNextProgrammsPage();
  void MoveToPreviousProgrammsPage();

  bool GetCurrentUrl(PlaylistEntry* url) const;

  void SwitchToPlayingMode();
  void SwitchToConnectMode();
  void SwitchToDisconnectMode();
  void SwitchToAuthorizeMode();
  void SwitchToUnAuthorizeMode();

  core::VideoState* CreateNextStream();
  core::VideoState* CreatePrevStream();
  core::VideoState* CreateStreamPos(size_t pos);

  size_t GenerateNextPosition() const;
  size_t GeneratePrevPosition() const;

  void MoveToNextStream();
  void MoveToPreviousStream();

  bool FindStreamByPoint(SDL_Point point, size_t* pos) const;
  bool IsHideButtonProgramsListRect(SDL_Point point) const;
  bool IsShowButtonProgramsListRect(SDL_Point point) const;

  SurfaceSaver* offline_channel_texture_;
  SurfaceSaver* connection_error_texture_;

  SurfaceSaver* hide_button_texture_;
  SurfaceSaver* show_button_texture_;

  IoService* controller_;

  size_t current_stream_pos_;
  std::vector<PlaylistEntry> play_list_;

  bool show_footer_;
  core::msec_t footer_last_shown_;
  std::string current_state_str_;

  const core::AppOptions opt_;
  const core::ComplexOptions copt_;

  const std::string app_directory_absolute_path_;

  bool show_keypad_;
  core::msec_t keypad_last_shown_;
  keypad_sym_t keypad_sym_;

  bool show_programms_list_;
  int last_programms_line_;
};

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
