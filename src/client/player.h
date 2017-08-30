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

#include "client/player/isimple_player.h"
#include "client/playlist_entry.h"

#include "client/player/gui/network_events.h"  // for BandwidthEstimationEvent

namespace fastotv {
namespace client {

class IoService;
class ChatWindow;

struct ChannelDescription {
  size_t pos;
  std::string title;
  std::string description;
  channel_icon_t icon;
};

class Player : public player::ISimplePlayer {
 public:
  static const SDL_Color failed_color;
  static const SDL_Color playlist_color;
  static const SDL_Color info_channel_color;
  static const SDL_Color chat_color;
  static const SDL_Color keypad_color;
  static const SDL_Color playlist_item_preselect_color;

  typedef ISimplePlayer base_class;
  typedef std::string keypad_sym_t;
  enum {
    footer_height = 60,
    chat_user_name_width = 120,
    keypad_height = 30,
    keypad_width = 60,
    min_key_pad_size = 0,
    max_keypad_size = 999
  };
  Player(const std::string& app_directory_absolute_path,  // for runtime data (cache)
         const player::PlayerOptions& options,
         const player::core::AppOptions& opt,
         const player::core::ComplexOptions& copt);

  ~Player();

  virtual std::string GetCurrentUrlName() const override;  // return Unknown if not found
  player::core::AppOptions GetStreamOptions() const;

 protected:
  virtual void HandleEvent(event_t* event) override;
  virtual void HandleExceptionEvent(event_t* event, common::Error err) override;

  virtual void HandlePreExecEvent(player::core::events::PreExecEvent* event) override;
  virtual void HandlePostExecEvent(player::core::events::PostExecEvent* event) override;

  virtual void HandleTimerEvent(player::core::events::TimerEvent* event) override;

  virtual void HandleBandwidthEstimationEvent(player::core::events::BandwidthEstimationEvent* event);
  virtual void HandleClientConnectedEvent(player::core::events::ClientConnectedEvent* event);
  virtual void HandleClientDisconnectedEvent(player::core::events::ClientDisconnectedEvent* event);
  virtual void HandleClientAuthorizedEvent(player::core::events::ClientAuthorizedEvent* event);
  virtual void HandleClientUnAuthorizedEvent(player::core::events::ClientUnAuthorizedEvent* event);
  virtual void HandleClientConfigChangeEvent(player::core::events::ClientConfigChangeEvent* event);
  virtual void HandleReceiveChannelsEvent(player::core::events::ReceiveChannelsEvent* event);
  virtual void HandleReceiveRuntimeChannelEvent(player::core::events::ReceiveRuntimeChannelEvent* event);
  virtual void HandleSendChatMessageEvent(player::core::events::SendChatMessageEvent* event);
  virtual void HandleReceiveChatMessageEvent(player::core::events::ReceiveChatMessageEvent* event);

  virtual void HandleWindowResizeEvent(player::core::events::WindowResizeEvent* event) override;
  virtual void HandleWindowExposeEvent(player::core::events::WindowExposeEvent* event) override;

  virtual void HandleMousePressEvent(player::core::events::MousePressEvent* event) override;

  virtual void HandleKeyPressEvent(player::core::events::KeyPressEvent* event) override;
  virtual void HandleLircPressEvent(player::core::events::LircPressEvent* event) override;

  virtual void DrawInfo() override;
  virtual void DrawFailedStatus() override;
  virtual void DrawInitStatus() override;

  virtual void InitWindow(const std::string& title, States status) override;

  virtual player::core::VideoState* CreateStream(stream_id sid,
                                                 const common::uri::Uri& uri,
                                                 player::core::AppOptions opt,
                                                 player::core::ComplexOptions copt) override;

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
  void DrawChat();

  void StartShowFooter();
  SDL_Rect GetFooterRect() const;

  void ToggleShowProgramsList();
  void ToggleShowChat();
  SDL_Rect GetProgramsListRect() const;
  SDL_Rect GetChatRect() const;

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

  player::core::VideoState* CreateNextStream();
  player::core::VideoState* CreatePrevStream();
  player::core::VideoState* CreateStreamPos(size_t pos);

  size_t GenerateNextPosition() const;
  size_t GeneratePrevPosition() const;

  void MoveToNextStream();
  void MoveToPreviousStream();

  bool FindStreamByPoint(SDL_Point point, size_t* pos) const;
  bool IsHideButtonProgramsListRect(const SDL_Point& point) const;
  bool IsShowButtonProgramsListRect(const SDL_Point& point) const;

  player::draw::SurfaceSaver* offline_channel_texture_;
  player::draw::SurfaceSaver* connection_error_texture_;

  player::draw::SurfaceSaver* right_arrow_button_texture_;
  player::draw::SurfaceSaver* left_arrow_button_texture_;

  IoService* controller_;

  size_t current_stream_pos_;
  std::vector<PlaylistEntry> play_list_;

  bool show_footer_;
  player::core::msec_t footer_last_shown_;
  std::string current_state_str_;

  const player::core::AppOptions opt_;
  const player::core::ComplexOptions copt_;

  const std::string app_directory_absolute_path_;

  bool show_keypad_;
  player::core::msec_t keypad_last_shown_;
  keypad_sym_t keypad_sym_;

  bool show_programms_list_;
  int last_programms_line_;

  ChatWindow* chat_window_;
};

}  // namespace client
}  // namespace fastotv
