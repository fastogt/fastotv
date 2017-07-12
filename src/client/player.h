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

#include "client/simple_player.h"

namespace fasto {
namespace fastotv {
namespace client {

class IoService;

class Player : public SimplePlayer {
 public:
  typedef SimplePlayer base_class;
  Player(const std::string& app_directory_absolute_path,
         const PlayerOptions& options,
         const core::AppOptions& opt,
         const core::ComplexOptions& copt);

  ~Player();

 protected:
  virtual void HandleEvent(event_t* event) override;
  virtual void HandleExceptionEvent(event_t* event, common::Error err) override;

  virtual void HandlePreExecEvent(core::events::PreExecEvent* event) override;
  virtual void HandlePostExecEvent(core::events::PostExecEvent* event) override;

  virtual void HandleBandwidthEstimationEvent(core::events::BandwidthEstimationEvent* event);
  virtual void HandleClientConnectedEvent(core::events::ClientConnectedEvent* event);
  virtual void HandleClientDisconnectedEvent(core::events::ClientDisconnectedEvent* event);
  virtual void HandleClientAuthorizedEvent(core::events::ClientAuthorizedEvent* event);
  virtual void HandleClientUnAuthorizedEvent(core::events::ClientUnAuthorizedEvent* event);
  virtual void HandleClientConfigChangeEvent(core::events::ClientConfigChangeEvent* event);
  virtual void HandleReceiveChannelsEvent(core::events::ReceiveChannelsEvent* event);

  virtual void DrawFailedStatus() override;
  virtual void DrawInitStatus() override;

 private:
  void SwitchToConnectMode();
  void SwitchToDisconnectMode();

  void SwitchToAuthorizeMode();
  void SwitchToUnAuthorizeMode();

  SurfaceSaver* offline_channel_texture_;
  SurfaceSaver* connection_error_texture_;

  IoService* controller_;
};

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
