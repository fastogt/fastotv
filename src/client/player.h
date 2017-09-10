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

#include <player/isimple_player.h>

#include "client/playlist_entry.h"
#include "client/events/network_events.h"  // for BandwidthEstimationEvent

namespace fastoplayer {
namespace gui {
class IconLabel;
class Button;
}  // namespace gui
}  // namespace player

namespace fastotv {
namespace client {

class IoService;
class ChatWindow;
class PlaylistWindow;

class Player : public fastoplayer::ISimplePlayer {
 public:
  static const SDL_Color failed_color;
  static const SDL_Color playlist_color;
  static const SDL_Color info_channel_color;
  static const SDL_Color chat_color;
  static const SDL_Color keypad_color;
  static const SDL_Color playlist_item_preselect_color;

  typedef fastoplayer::ISimplePlayer base_class;
  enum { footer_height = 60, keypad_height = 30, keypad_width = 60, min_key_pad_size = 0, max_keypad_size = 999 };
  Player(const std::string& app_directory_absolute_path,  // for runtime data (cache)
         const fastoplayer::PlayerOptions& options,
         const fastoplayer::media::AppOptions& opt,
         const fastoplayer::media::ComplexOptions& copt);

  ~Player();

  virtual std::string GetCurrentUrlName() const override;  // return Unknown if not found
  fastoplayer::media::AppOptions GetStreamOptions() const;

 protected:
  virtual void HandleEvent(event_t* event) override;
  virtual void HandleExceptionEvent(event_t* event, common::Error err) override;

  virtual void HandlePreExecEvent(fastoplayer::gui::events::PreExecEvent* event) override;
  virtual void HandlePostExecEvent(fastoplayer::gui::events::PostExecEvent* event) override;

  virtual void HandleTimerEvent(fastoplayer::gui::events::TimerEvent* event) override;

  virtual void HandleBandwidthEstimationEvent(events::BandwidthEstimationEvent* event);
  virtual void HandleClientConnectedEvent(events::ClientConnectedEvent* event);
  virtual void HandleClientDisconnectedEvent(events::ClientDisconnectedEvent* event);
  virtual void HandleClientAuthorizedEvent(events::ClientAuthorizedEvent* event);
  virtual void HandleClientUnAuthorizedEvent(events::ClientUnAuthorizedEvent* event);
  virtual void HandleClientConfigChangeEvent(events::ClientConfigChangeEvent* event);
  virtual void HandleReceiveChannelsEvent(events::ReceiveChannelsEvent* event);
  virtual void HandleReceiveRuntimeChannelEvent(events::ReceiveRuntimeChannelEvent* event);
  virtual void HandleSendChatMessageEvent(events::SendChatMessageEvent* event);
  virtual void HandleReceiveChatMessageEvent(events::ReceiveChatMessageEvent* event);

  virtual void HandleKeyPressEvent(fastoplayer::gui::events::KeyPressEvent* event) override;
  virtual void HandleLircPressEvent(fastoplayer::gui::events::LircPressEvent* event) override;

  virtual void DrawInfo() override;
  virtual void DrawFailedStatus() override;
  virtual void DrawInitStatus() override;

  virtual void InitWindow(const std::string& title, States status) override;
  virtual void SetStatus(States new_state) override;

  virtual fastoplayer::media::VideoState* CreateStream(stream_id sid,
                                                       const common::uri::Url& uri,
                                                       fastoplayer::media::AppOptions opt,
                                                       fastoplayer::media::ComplexOptions copt) override;

  virtual void OnWindowCreated(SDL_Window* window, SDL_Renderer* render) override;

 private:
  void SetVisiblePlaylist(bool visible);
  void SetVisibleChat(bool visible);

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
  SDL_Rect GetHideButtonPlayListRect() const;
  SDL_Rect GetShowButtonPlayListRect() const;
  SDL_Rect GetHideButtonChatRect() const;
  SDL_Rect GetShowButtonChatRect() const;

  bool GetCurrentUrl(PlaylistEntry* url) const;

  void SwitchToPlayingMode();
  void SwitchToConnectMode();
  void SwitchToDisconnectMode();
  void SwitchToAuthorizeMode();
  void SwitchToUnAuthorizeMode();

  fastoplayer::media::VideoState* CreateNextStream();
  fastoplayer::media::VideoState* CreatePrevStream();
  fastoplayer::media::VideoState* CreateStreamPos(size_t pos);

  size_t GenerateNextPosition() const;
  size_t GeneratePrevPosition() const;

  void MoveToNextStream();
  void MoveToPreviousStream();

  fastoplayer::draw::SurfaceSaver* offline_channel_texture_;
  fastoplayer::draw::SurfaceSaver* connection_error_texture_;

  fastoplayer::draw::SurfaceSaver* right_arrow_button_texture_;
  fastoplayer::draw::SurfaceSaver* left_arrow_button_texture_;
  fastoplayer::draw::SurfaceSaver* up_arrow_button_texture_;
  fastoplayer::draw::SurfaceSaver* down_arrow_button_texture_;

  fastoplayer::gui::Button* show_playlist_button_;
  fastoplayer::gui::Button* hide_playlist_button_;

  fastoplayer::gui::Button* show_chat_button_;
  fastoplayer::gui::Button* hide_chat_button_;

  IoService* controller_;

  size_t current_stream_pos_;
  std::vector<PlaylistEntry> play_list_;

  fastoplayer::gui::IconLabel* description_label_;
  fastoplayer::media::msec_t footer_last_shown_;

  const fastoplayer::media::AppOptions opt_;
  const fastoplayer::media::ComplexOptions copt_;

  const std::string app_directory_absolute_path_;

  fastoplayer::gui::Label* keypad_label_;
  fastoplayer::media::msec_t keypad_last_shown_;

  PlaylistWindow* plailist_window_;
  ChatWindow* chat_window_;

  AuthInfo auth_;
};

}  // namespace client
}  // namespace fastotv
