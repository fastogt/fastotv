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

#include "auth_info.h"

#include "client/isimple_player.h"
#include "client/playlist_entry.h"

#include "client/core/events/network_events.h"  // for BandwidthEstimationEvent

namespace fasto {
namespace fastotv {
namespace client {

class IoService;

class Player : public ISimplePlayer {
 public:
  typedef ISimplePlayer base_class;
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

  virtual void HandleKeyPressEvent(core::events::KeyPressEvent* event) override;
  virtual void HandleLircPressEvent(core::events::LircPressEvent* event) override;

  virtual void DrawInfo() override;
  virtual void DrawFailedStatus() override;
  virtual void DrawInitStatus() override;

  virtual void InitWindow(const std::string& title, States status) override;

 private:
  void DrawFooter();

  void StartShowFooter();
  SDL_Rect GetFooterRect() const;

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

  SurfaceSaver* offline_channel_texture_;
  SurfaceSaver* connection_error_texture_;

  IoService* controller_;

  size_t current_stream_pos_;
  std::vector<PlaylistEntry> play_list_;

  bool show_footer_;
  core::msec_t footer_last_shown_;
  std::string current_state_str_;

  const core::AppOptions opt_;
  const core::ComplexOptions copt_;

  const std::string app_directory_absolute_path_;
};

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
