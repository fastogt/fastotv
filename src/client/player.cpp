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

#include "client/player.h"

#include <common/application/application.h>
#include <common/convert2string.h>
#include <common/file_system.h>
#include <common/threads/thread_manager.h>
#include <common/utils.h>

#include "client/ioservice.h"  // for IoService
#include "client/utils.h"

#include "client/player/sdl_utils.h"  // for IMG_LoadPNG, SurfaceSaver

#include "client/player/draw/surface_saver.h"

// widgets
#include "client/player/gui/widgets/icon_label.h"

#include "client/chat_window.h"
#include "client/playlist_window.h"

#define IMG_OFFLINE_CHANNEL_PATH_RELATIVE "share/resources/offline_channel.png"
#define IMG_CONNECTION_ERROR_PATH_RELATIVE "share/resources/connection_error.png"

#define IMG_HIDE_BUTTON_PATH_RELATIVE "share/resources/right_arrow.png"
#define IMG_SHOW_BUTTON_PATH_RELATIVE "share/resources/left_arrow.png"

#define CACHE_FOLDER_NAME "cache"

#define FOOTER_HIDE_DELAY_MSEC 2000  // 2 sec
#define KEYPAD_HIDE_DELAY_MSEC 3000  // 3 sec

namespace fastotv {
namespace client {

namespace {

player::draw::SurfaceSaver* MakeSurfaceFromImageRelativePath(const std::string& relative_path) {
  const std::string absolute_source_dir = common::file_system::absolute_path_from_relative(RELATIVE_SOURCE_DIR);
  const std::string img_full_path = common::file_system::make_path(absolute_source_dir, relative_path);
  return player::draw::MakeSurfaceFromPath(img_full_path);
}

}  // namespace

const SDL_Color Player::failed_color = {193, 66, 66, Uint8(SDL_ALPHA_OPAQUE * 0.5)};
const SDL_Color Player::playlist_color = {98, 118, 217, Uint8(SDL_ALPHA_OPAQUE * 0.5)};
const SDL_Color Player::keypad_color = player::ISimplePlayer::stream_statistic_color;
const SDL_Color Player::playlist_item_preselect_color = {193, 66, 66, Uint8(SDL_ALPHA_OPAQUE * 0.1)};
const SDL_Color Player::chat_color = {98, 118, 217, Uint8(SDL_ALPHA_OPAQUE * 0.5)};
const SDL_Color Player::info_channel_color = {98, 118, 217, Uint8(SDL_ALPHA_OPAQUE * 0.5)};

Player::Player(const std::string& app_directory_absolute_path,
               const player::PlayerOptions& options,
               const player::media::AppOptions& opt,
               const player::media::ComplexOptions& copt)
    : ISimplePlayer(options),
      offline_channel_texture_(nullptr),
      connection_error_texture_(nullptr),
      right_arrow_button_texture_(nullptr),
      left_arrow_button_texture_(nullptr),
      controller_(new IoService),
      current_stream_pos_(0),
      play_list_(),
      description_label_(nullptr),
      footer_last_shown_(0),
      opt_(opt),
      copt_(copt),
      app_directory_absolute_path_(app_directory_absolute_path),
      keypad_label_(nullptr),
      keypad_last_shown_(0),
      keypad_sym_(),
      plailist_window_(nullptr),
      chat_window_(nullptr) {
  fApp->Subscribe(this, player::gui::events::BandwidthEstimationEvent::EventType);

  fApp->Subscribe(this, player::gui::events::ClientDisconnectedEvent::EventType);
  fApp->Subscribe(this, player::gui::events::ClientConnectedEvent::EventType);

  fApp->Subscribe(this, player::gui::events::ClientAuthorizedEvent::EventType);
  fApp->Subscribe(this, player::gui::events::ClientUnAuthorizedEvent::EventType);

  fApp->Subscribe(this, player::gui::events::ClientConfigChangeEvent::EventType);
  fApp->Subscribe(this, player::gui::events::ReceiveChannelsEvent::EventType);
  fApp->Subscribe(this, player::gui::events::ReceiveRuntimeChannelEvent::EventType);
  fApp->Subscribe(this, player::gui::events::SendChatMessageEvent::EventType);
  fApp->Subscribe(this, player::gui::events::ReceiveChatMessageEvent::EventType);

  // chat window
  chat_window_ = new ChatWindow;
  chat_window_->SetVisible(false);
  chat_window_->SetBackGroundColor(chat_color);

  // descr window
  description_label_ = new player::gui::IconLabel;
  description_label_->SetVisible(false);
  description_label_->SetTextColor(text_color);
  description_label_->SetSpace(space_width);
  description_label_->SetText("Init");

  // key_pad window
  keypad_label_ = new player::gui::Label;
  keypad_label_->SetVisible(false);
  keypad_label_->SetBackGroundColor(keypad_color);
  keypad_label_->SetTextColor(text_color);
  keypad_label_->SetDrawType(player::gui::Label::CENTER_TEXT);

  // playlist window
  plailist_window_ = new PlaylistWindow;
  plailist_window_->SetVisible(false);
  plailist_window_->SetBackGroundColor(playlist_color);
  plailist_window_->SetSelection(PlaylistWindow::SINGLE_ROW_SELECT);
  plailist_window_->SetSelectionColor(playlist_item_preselect_color);
  plailist_window_->SetDrawType(player::gui::Label::WRAPPED_TEXT);
  plailist_window_->SetTextColor(text_color);
  plailist_window_->SetCurrentPositionSelectionColor(failed_color);
}

Player::~Player() {
  destroy(&plailist_window_);
  destroy(&keypad_label_);
  destroy(&description_label_);
  destroy(&chat_window_);
  destroy(&controller_);
}

void Player::HandleEvent(event_t* event) {
  if (event->GetEventType() == player::gui::events::BandwidthEstimationEvent::EventType) {
    player::gui::events::BandwidthEstimationEvent* band_event =
        static_cast<player::gui::events::BandwidthEstimationEvent*>(event);
    HandleBandwidthEstimationEvent(band_event);
  } else if (event->GetEventType() == player::gui::events::ClientConnectedEvent::EventType) {
    player::gui::events::ClientConnectedEvent* connect_event =
        static_cast<player::gui::events::ClientConnectedEvent*>(event);
    HandleClientConnectedEvent(connect_event);
  } else if (event->GetEventType() == player::gui::events::ClientDisconnectedEvent::EventType) {
    player::gui::events::ClientDisconnectedEvent* disc_event =
        static_cast<player::gui::events::ClientDisconnectedEvent*>(event);
    HandleClientDisconnectedEvent(disc_event);
  } else if (event->GetEventType() == player::gui::events::ClientAuthorizedEvent::EventType) {
    player::gui::events::ClientAuthorizedEvent* auth_event =
        static_cast<player::gui::events::ClientAuthorizedEvent*>(event);
    HandleClientAuthorizedEvent(auth_event);
  } else if (event->GetEventType() == player::gui::events::ClientUnAuthorizedEvent::EventType) {
    player::gui::events::ClientUnAuthorizedEvent* unauth_event =
        static_cast<player::gui::events::ClientUnAuthorizedEvent*>(event);
    HandleClientUnAuthorizedEvent(unauth_event);
  } else if (event->GetEventType() == player::gui::events::ClientConfigChangeEvent::EventType) {
    player::gui::events::ClientConfigChangeEvent* conf_change_event =
        static_cast<player::gui::events::ClientConfigChangeEvent*>(event);
    HandleClientConfigChangeEvent(conf_change_event);
  } else if (event->GetEventType() == player::gui::events::ReceiveChannelsEvent::EventType) {
    player::gui::events::ReceiveChannelsEvent* channels_event =
        static_cast<player::gui::events::ReceiveChannelsEvent*>(event);
    HandleReceiveChannelsEvent(channels_event);
  } else if (event->GetEventType() == player::gui::events::ReceiveRuntimeChannelEvent::EventType) {
    player::gui::events::ReceiveRuntimeChannelEvent* channel_event =
        static_cast<player::gui::events::ReceiveRuntimeChannelEvent*>(event);
    HandleReceiveRuntimeChannelEvent(channel_event);
  } else if (event->GetEventType() == player::gui::events::SendChatMessageEvent::EventType) {
    player::gui::events::SendChatMessageEvent* chat_msg_event =
        static_cast<player::gui::events::SendChatMessageEvent*>(event);
    HandleSendChatMessageEvent(chat_msg_event);
  } else if (event->GetEventType() == player::gui::events::ReceiveChatMessageEvent::EventType) {
    player::gui::events::ReceiveChatMessageEvent* chat_msg_event =
        static_cast<player::gui::events::ReceiveChatMessageEvent*>(event);
    HandleReceiveChatMessageEvent(chat_msg_event);
  }

  base_class::HandleEvent(event);
}

void Player::HandleExceptionEvent(event_t* event, common::Error err) {
  if (event->GetEventType() == player::gui::events::ClientConnectedEvent::EventType) {
    // gui::events::ClientConnectedEvent* connect_event =
    //    static_cast<gui::events::ClientConnectedEvent*>(event);
    SwitchToDisconnectMode();
  } else if (event->GetEventType() == player::gui::events::ClientAuthorizedEvent::EventType) {
    // gui::events::ClientConnectedEvent* connect_event =
    //    static_cast<gui::events::ClientConnectedEvent*>(event);
    SwitchToUnAuthorizeMode();
  } else if (event->GetEventType() == player::gui::events::BandwidthEstimationEvent::EventType) {
    player::gui::events::BandwidthEstimationEvent* band_event =
        static_cast<player::gui::events::BandwidthEstimationEvent*>(event);
    HandleBandwidthEstimationEvent(band_event);
  }

  base_class::HandleExceptionEvent(event, err);
}

void Player::HandlePreExecEvent(player::gui::events::PreExecEvent* event) {
  player::gui::events::PreExecInfo inf = event->GetInfo();
  if (inf.code == EXIT_SUCCESS) {
    offline_channel_texture_ = MakeSurfaceFromImageRelativePath(IMG_OFFLINE_CHANNEL_PATH_RELATIVE);
    connection_error_texture_ = MakeSurfaceFromImageRelativePath(IMG_CONNECTION_ERROR_PATH_RELATIVE);
    right_arrow_button_texture_ = MakeSurfaceFromImageRelativePath(IMG_HIDE_BUTTON_PATH_RELATIVE);
    left_arrow_button_texture_ = MakeSurfaceFromImageRelativePath(IMG_SHOW_BUTTON_PATH_RELATIVE);

    controller_->Start();
    SwitchToConnectMode();
  }

  base_class::HandlePreExecEvent(event);
  TTF_Font* font = GetFont();
  int h = player::draw::CalcHeightFontPlaceByRowCount(font, 2);

  chat_window_->SetFont(font);
  chat_window_->SetRowHeight(h);
  description_label_->SetFont(font);
  keypad_label_->SetFont(font);
  plailist_window_->SetFont(font);
  plailist_window_->SetRowHeight(h);
}

void Player::HandleTimerEvent(player::gui::events::TimerEvent* event) {
  player::media::msec_t cur_time = player::media::GetCurrentMsec();
  player::media::msec_t diff_footer = cur_time - footer_last_shown_;
  if (description_label_->IsVisible() && diff_footer > FOOTER_HIDE_DELAY_MSEC) {
    description_label_->SetVisible(false);
  }

  player::media::msec_t diff_keypad = cur_time - keypad_last_shown_;
  if (keypad_label_->IsVisible() && diff_keypad > KEYPAD_HIDE_DELAY_MSEC) {
    ResetKeyPad();
  }

  base_class::HandleTimerEvent(event);
}

void Player::HandlePostExecEvent(player::gui::events::PostExecEvent* event) {
  player::gui::events::PostExecInfo inf = event->GetInfo();
  if (inf.code == EXIT_SUCCESS) {
    controller_->Stop();
    destroy(&offline_channel_texture_);
    destroy(&connection_error_texture_);
    destroy(&right_arrow_button_texture_);
    destroy(&left_arrow_button_texture_);
    play_list_.clear();
  }
  base_class::HandlePostExecEvent(event);
}

std::string Player::GetCurrentUrlName() const {
  PlaylistEntry url;
  if (GetCurrentUrl(&url)) {
    ChannelInfo ch = url.GetChannelInfo();
    return ch.GetName();
  }

  return "Unknown";
}

player::media::AppOptions Player::GetStreamOptions() const {
  return opt_;
}

bool Player::GetCurrentUrl(PlaylistEntry* url) const {
  if (!url || play_list_.empty()) {
    return false;
  }

  *url = play_list_[current_stream_pos_];
  return true;
}

void Player::SwitchToPlayingMode() {
  if (play_list_.empty()) {
    return;
  }

  player::PlayerOptions opt = GetOptions();

  size_t pos = current_stream_pos_;
  for (size_t i = 0; i < play_list_.size() && opt.last_showed_channel_id != invalid_stream_id; ++i) {
    PlaylistEntry ent = play_list_[i];
    ChannelInfo ch = ent.GetChannelInfo();
    if (ch.GetId() == opt.last_showed_channel_id) {
      pos = i;
      break;
    }
  }

  player::media::VideoState* stream = CreateStreamPos(pos);
  SetStream(stream);
}

void Player::SwitchToAuthorizeMode() {
  InitWindow("Authorize...", INIT_STATE);
}

void Player::SwitchToUnAuthorizeMode() {
  InitWindow("UnAuthorize...", INIT_STATE);
}

void Player::SwitchToConnectMode() {
  InitWindow("Connecting...", INIT_STATE);
}

void Player::SwitchToDisconnectMode() {
  InitWindow("Disconnected", INIT_STATE);
}

void Player::HandleBandwidthEstimationEvent(player::gui::events::BandwidthEstimationEvent* event) {
  player::gui::events::BandwidtInfo band_inf = event->GetInfo();
  if (band_inf.host_type == MAIN_SERVER) {
    controller_->RequestChannels();
  }
}

void Player::HandleClientConnectedEvent(player::gui::events::ClientConnectedEvent* event) {
  UNUSED(event);
  SwitchToAuthorizeMode();
}

void Player::HandleClientDisconnectedEvent(player::gui::events::ClientDisconnectedEvent* event) {
  UNUSED(event);
  if (GetCurrentState() == INIT_STATE) {
    SwitchToDisconnectMode();
  }
}

void Player::HandleClientAuthorizedEvent(player::gui::events::ClientAuthorizedEvent* event) {
  UNUSED(event);

  controller_->RequestServerInfo();
}

void Player::HandleClientUnAuthorizedEvent(player::gui::events::ClientUnAuthorizedEvent* event) {
  UNUSED(event);
  if (GetCurrentState() == INIT_STATE) {
    SwitchToDisconnectMode();
  }
}

void Player::HandleClientConfigChangeEvent(player::gui::events::ClientConfigChangeEvent* event) {
  UNUSED(event);
}

void Player::HandleReceiveChannelsEvent(player::gui::events::ReceiveChannelsEvent* event) {
  ChannelsInfo chan = event->GetInfo();
  // prepare cache folders
  ChannelsInfo::channels_t channels = chan.GetChannels();
  const std::string cache_dir = common::file_system::make_path(app_directory_absolute_path_, CACHE_FOLDER_NAME);
  bool is_exist_cache_root = common::file_system::is_directory_exist(cache_dir);
  if (!is_exist_cache_root) {
    common::ErrnoError err = common::file_system::create_directory(cache_dir, true);
    if (err && err->IsError()) {
      DEBUG_MSG_ERROR(err);
    } else {
      is_exist_cache_root = true;
    }
  }

  for (const ChannelInfo& ch : channels) {
    PlaylistEntry entry = PlaylistEntry(cache_dir, ch);
    const std::string icon_path = entry.GetIconPath();
    player::draw::SurfaceSaver* surf = player::draw::MakeSurfaceFromPath(icon_path);
    channel_icon_t shared_surface(surf);
    entry.SetIcon(shared_surface);
    play_list_.push_back(entry);

    if (is_exist_cache_root) {  // prepare cache folders for channels
      const std::string channel_dir = entry.GetCacheDir();
      bool is_cache_channel_dir_exist = common::file_system::is_directory_exist(channel_dir);
      if (!is_cache_channel_dir_exist) {
        common::ErrnoError err = common::file_system::create_directory(channel_dir, true);
        if (err && err->IsError()) {
          DEBUG_MSG_ERROR(err);
        } else {
          is_cache_channel_dir_exist = true;
        }
      }

      if (!is_cache_channel_dir_exist) {
        continue;
      }

      EpgInfo epg = ch.GetEpg();
      common::uri::Uri uri = epg.GetIconUrl();
      bool is_unknown_icon = EpgInfo::IsUnknownIconUrl(uri);
      if (is_unknown_icon) {
        continue;
      }

      auto load_image_cb = [entry, uri, channel_dir]() {
        const std::string channel_icon_path = entry.GetIconPath();
        if (!common::file_system::is_file_exist(channel_icon_path)) {  // if not exist trying to download
          common::buffer_t buff;
          bool is_file_downloaded = DownloadFileToBuffer(uri, &buff);
          if (!is_file_downloaded) {
            return;
          }

          const uint32_t fl = common::file_system::File::FLAG_CREATE | common::file_system::File::FLAG_WRITE |
                              common::file_system::File::FLAG_OPEN_BINARY;
          common::file_system::File channel_icon_file;
          common::ErrnoError err = channel_icon_file.Open(channel_icon_path, fl);
          if (err && err->IsError()) {
            DEBUG_MSG_ERROR(err);
            return;
          }

          size_t writed;
          err = channel_icon_file.Write(buff, &writed);
          if (err && err->IsError()) {
            DEBUG_MSG_ERROR(err);
            err = channel_icon_file.Close();
            return;
          }

          err = channel_icon_file.Close();
        }
      };
      controller_->ExecInLoopThread(load_image_cb);
    }
  }

  plailist_window_->SetVisible(true);
  plailist_window_->SetPlaylist(&play_list_);
  SwitchToPlayingMode();
}

void Player::HandleReceiveRuntimeChannelEvent(player::gui::events::ReceiveRuntimeChannelEvent* event) {
  RuntimeChannelInfo inf = event->GetInfo();
  for (size_t i = 0; i < play_list_.size(); ++i) {
    ChannelInfo cinf = play_list_[i].GetChannelInfo();
    if (inf.GetChannelId() == cinf.GetId()) {
      play_list_[i].SetRuntimeChannelInfo(inf);
      break;
    }
  }
}

void Player::HandleSendChatMessageEvent(player::gui::events::SendChatMessageEvent* event) {
  UNUSED(event);
}

void Player::HandleReceiveChatMessageEvent(player::gui::events::ReceiveChatMessageEvent* event) {
  UNUSED(event);
}

void Player::HandleWindowResizeEvent(player::gui::events::WindowResizeEvent* event) {
  base_class::HandleWindowResizeEvent(event);
}

void Player::HandleWindowExposeEvent(player::gui::events::WindowExposeEvent* event) {
  base_class::HandleWindowExposeEvent(event);
}

void Player::HandleMousePressEvent(player::gui::events::MousePressEvent* event) {
  player::PlayerOptions opt = GetOptions();
  if (opt.exit_on_mousedown) {
    return base_class::HandleMousePressEvent(event);
  }

  player::gui::events::MousePressInfo inf = event->GetInfo();
  SDL_MouseButtonEvent sinfo = inf.mevent;
  if (sinfo.button == SDL_BUTTON_LEFT) {
    if (plailist_window_->IsVisible()) {
      SDL_Point point = inf.GetMousePoint();
      const SDL_Rect programms_list_rect = GetProgramsListRect();
      if (player::draw::IsPointInRect(point, programms_list_rect)) {
        size_t pos = plailist_window_->GetActiveRow();
        if (pos != player::draw::invalid_row_position) {  // pos in playlist
          player::media::VideoState* stream = CreateStreamPos(pos);
          SetStream(stream);
        }
      }
    }
  }
  base_class::HandleMousePressEvent(event);
}

void Player::HandleKeyPressEvent(player::gui::events::KeyPressEvent* event) {
  player::PlayerOptions opt = GetOptions();
  if (opt.exit_on_keydown) {
    return base_class::HandleKeyPressEvent(event);
  }

  const player::gui::events::KeyPressInfo inf = event->GetInfo();
  const SDL_Scancode scan_code = inf.ks.scancode;
  const Uint16 modifier = inf.ks.mod;
  if (modifier == KMOD_NONE) {
    if (scan_code == SDL_SCANCODE_KP_0) {
      HandleKeyPad(0);
    } else if (scan_code == SDL_SCANCODE_KP_1) {
      HandleKeyPad(1);
    } else if (scan_code == SDL_SCANCODE_KP_2) {
      HandleKeyPad(2);
    } else if (scan_code == SDL_SCANCODE_KP_3) {
      HandleKeyPad(3);
    } else if (scan_code == SDL_SCANCODE_KP_4) {
      HandleKeyPad(4);
    } else if (scan_code == SDL_SCANCODE_KP_5) {
      HandleKeyPad(5);
    } else if (scan_code == SDL_SCANCODE_KP_6) {
      HandleKeyPad(6);
    } else if (scan_code == SDL_SCANCODE_KP_7) {
      HandleKeyPad(7);
    } else if (scan_code == SDL_SCANCODE_KP_8) {
      HandleKeyPad(8);
    } else if (scan_code == SDL_SCANCODE_KP_9) {
      HandleKeyPad(9);
    } else if (scan_code == SDL_SCANCODE_BACKSPACE) {
      RemoveLastSymbolInKeypad();
    } else if (scan_code == SDL_SCANCODE_KP_ENTER) {
      FinishKeyPadInput();
    } else if (scan_code == SDL_SCANCODE_F4) {
      StartShowFooter();
    } else if (scan_code == SDL_SCANCODE_F5) {
      ToggleShowProgramsList();
    } else if (scan_code == SDL_SCANCODE_F6) {
      ToggleShowChat();
    } else if (scan_code == SDL_SCANCODE_UP) {
      MoveToPreviousStream();
    } else if (scan_code == SDL_SCANCODE_DOWN) {
      MoveToNextStream();
    }
  }

  base_class::HandleKeyPressEvent(event);
}

void Player::HandleLircPressEvent(player::gui::events::LircPressEvent* event) {
  player::PlayerOptions opt = GetOptions();
  if (opt.exit_on_keydown) {
    return base_class::HandleLircPressEvent(event);
  }

  player::gui::events::LircPressInfo inf = event->GetInfo();
  if (inf.code == LIRC_KEY_LEFT) {
    MoveToPreviousStream();
  } else if (inf.code == LIRC_KEY_RIGHT) {
    MoveToNextStream();
  }

  base_class::HandleLircPressEvent(event);
}

void Player::DrawInfo() {
  DrawFooter();
  DrawKeyPad();
  DrawProgramsList();
  DrawChat();
  base_class::DrawInfo();
}

SDL_Rect Player::GetFooterRect() const {
  const SDL_Rect display_rect = GetDrawRect();
  return {display_rect.x, display_rect.h - footer_height - volume_height - space_height + display_rect.y,
          display_rect.w, footer_height};
}

void Player::ToggleShowProgramsList() {
  plailist_window_->SetVisible(!plailist_window_->IsVisible());
}

void Player::ToggleShowChat() {
  chat_window_->SetVisible(!chat_window_->IsVisible());
}

SDL_Rect Player::GetProgramsListRect() const {
  const SDL_Rect display_rect = GetDisplayRect();
  return {display_rect.x + display_rect.w * 3 / 4, display_rect.y, display_rect.w / 4, display_rect.h};
}

SDL_Rect Player::GetChatRect() const {
  const SDL_Rect display_rect = GetDisplayRect();
  return {0, display_rect.y, display_rect.w / 4, display_rect.h};
}

bool Player::GetChannelDescription(size_t pos, ChannelDescription* descr) const {
  if (!descr || pos >= play_list_.size()) {
    DNOTREACHED();
    return false;
  }

  *descr = play_list_[pos].GetChannelDescription();
  return true;
}

void Player::HandleKeyPad(uint8_t key) {
  if (play_list_.empty()) {
    return;
  }

  size_t cur_number;
  if (!common::ConvertFromString(keypad_sym_, &cur_number)) {
    cur_number = 0;
  }

  keypad_label_->SetVisible(true);
  player::media::msec_t cur_time = player::media::GetCurrentMsec();
  keypad_last_shown_ = cur_time;
  size_t nex_keypad_sym = cur_number * 10 + key;
  if (nex_keypad_sym <= max_keypad_size) {
    keypad_sym_ = common::ConvertToString(nex_keypad_sym);
  }
}

void Player::RemoveLastSymbolInKeypad() {
  size_t cur_number;
  if (!common::ConvertFromString(keypad_sym_, &cur_number)) {
    return;
  }

  size_t nex_keypad_sym = cur_number / 10;
  if (nex_keypad_sym == 0) {
    ResetKeyPad();
    return;
  }

  keypad_sym_ = common::ConvertToString(nex_keypad_sym);
}

void Player::FinishKeyPadInput() {
  CHECK(THREAD_MANAGER()->IsMainThread());
  size_t pos;
  if (!common::ConvertFromString(keypad_sym_, &pos)) {
    return;
  }
  ResetKeyPad();

  CreateStreamPosAfterKeypad(pos);
}

void Player::CreateStreamPosAfterKeypad(size_t pos) {
  size_t stabled_pos = pos - 1;  // because start displaying channels from 1 in programms list
  if (stabled_pos >= play_list_.size()) {
    // error
    return;
  }

  player::media::VideoState* stream = CreateStreamPos(stabled_pos);
  SetStream(stream);
}

void Player::ResetKeyPad() {
  keypad_label_->SetVisible(false);
  keypad_sym_.clear();
}

SDL_Rect Player::GetKeyPadRect() const {
  const SDL_Rect display_rect = GetDrawRect();
  return {display_rect.w + space_width - keypad_width, display_rect.y, keypad_width, keypad_height};
}

int Player::GetMaxProgrammsLines() const {
  TTF_Font* font = GetFont();
  if (!font) {
    return 0;
  }

  const SDL_Rect programms_list_rect = GetProgramsListRect();
  int font_height_2line = player::draw::CalcHeightFontPlaceByRowCount(font, 2);
  return programms_list_rect.h / font_height_2line;
}

void Player::DrawProgramsList() {
  SDL_Renderer* render = GetRenderer();
  TTF_Font* font = GetFont();
  if (!font || !render) {
    return;
  }

  const SDL_Rect programms_list_rect = GetProgramsListRect();
  int font_height_2line = player::draw::CalcHeightFontPlaceByRowCount(font, 2);
  int min_size_width = keypad_width + font_height_2line + space_width + font_height_2line;  // number + icon + text
  if (programms_list_rect.w < min_size_width) {
    return;
  }

  plailist_window_->SetRect(programms_list_rect);
  plailist_window_->Draw(render);
}

void Player::DrawChat() {
  SDL_Renderer* render = GetRenderer();
  TTF_Font* font = GetFont();
  if (!font || !render) {
    return;
  }

  PlaylistEntry url;
  if (!GetCurrentUrl(&url)) {
    return;
  }

  RuntimeChannelInfo rinfo = url.GetRuntimeChannelInfo();
  if (!rinfo.IsChatEnabled()) {
    return;
  }

  const SDL_Rect chat_rect = GetChatRect();
  int min_size_width = chat_user_name_width + space_width + chat_user_name_width * 2;  // login + space + text
  if (chat_rect.w < min_size_width) {
    return;
  }

  chat_window_->SetRect(chat_rect);
  chat_window_->Draw(render);
}

void Player::DrawKeyPad() {
  SDL_Renderer* render = GetRenderer();
  TTF_Font* font = GetFont();
  if (!keypad_label_->IsVisible() || !font) {
    return;
  }

  const SDL_Rect keypad_rect = GetKeyPadRect();
  keypad_label_->SetText(keypad_sym_);
  keypad_label_->SetRect(keypad_rect);
  keypad_label_->Draw(render);
}

void Player::DrawFooter() {
  SDL_Renderer* render = GetRenderer();
  TTF_Font* font = GetFont();
  if (!description_label_->IsVisible() || !font) {
    return;
  }

  const SDL_Rect footer_rect = GetFooterRect();
  int padding_left = footer_rect.w / 4;
  SDL_Rect banner_footer_rect = {footer_rect.x + padding_left, footer_rect.y, footer_rect.w - padding_left * 2,
                                 footer_rect.h};
  description_label_->SetRect(banner_footer_rect);
  description_label_->Draw(render);
}

void Player::DrawFailedStatus() {
  SDL_Renderer* render = GetRenderer();
  if (!render) {
    return;
  }

  player::draw::FlushRender(render, black_color);
  if (offline_channel_texture_) {
    SDL_Texture* img = offline_channel_texture_->GetTexture(render);
    if (img) {
      SDL_RenderCopy(render, img, NULL, NULL);
    }
  }
  DrawInfo();
  SDL_RenderPresent(render);
}

void Player::InitWindow(const std::string& title, States status) {
  StartShowFooter();
  if (status != PLAYING_STATE) {
    description_label_->SetText(title);
  }

  base_class::InitWindow(title, status);
}

void Player::SetStatus(States new_state) {
  if (new_state == INIT_STATE) {
    description_label_->SetDrawType(player::gui::Label::CENTER_TEXT);
    description_label_->SetIconTexture(NULL);
    description_label_->SetBackGroundColor(failed_color);
  } else if (new_state == FAILED_STATE) {
    description_label_->SetDrawType(player::gui::Label::CENTER_TEXT);
    description_label_->SetIconTexture(NULL);
    description_label_->SetBackGroundColor(failed_color);
  } else if (new_state == PLAYING_STATE) {
    ChannelDescription descr;
    if (GetChannelDescription(current_stream_pos_, &descr)) {
#define DESCR_LINES_COUNT 2
      std::string footer_text = common::MemSPrintf(
          "Title: %s\n"
          "Description: %s",
          descr.title, descr.description);
      channel_icon_t icon = descr.icon;
      if (icon) {
        SDL_Renderer* render = GetRenderer();
        TTF_Font* font = GetFont();

        const SDL_Rect footer_rect = GetFooterRect();
        description_label_->SetIconTexture(icon->GetTexture(render));
        int h = player::draw::CalcHeightFontPlaceByRowCount(font, DESCR_LINES_COUNT);
        if (h > footer_rect.h) {
          h = footer_rect.h;
        }
        description_label_->SetIconSize(player::draw::Size(h, h));
      } else {
        description_label_->SetIconTexture(NULL);
      }
      description_label_->SetDrawType(player::gui::Label::WRAPPED_TEXT);
      description_label_->SetText(footer_text);
      description_label_->SetBackGroundColor(info_channel_color);
    }
  } else {
    NOTREACHED();
  }

  base_class::SetStatus(new_state);
}

void Player::DrawInitStatus() {
  SDL_Renderer* render = GetRenderer();
  if (!render) {
    return;
  }

  player::draw::FlushRender(render, black_color);
  if (connection_error_texture_) {
    SDL_Texture* img = connection_error_texture_->GetTexture(render);
    if (img) {
      SDL_RenderCopy(render, img, NULL, NULL);
    }
  }
  DrawInfo();
  SDL_RenderPresent(render);
}

player::media::VideoState* Player::CreateStream(stream_id sid,
                                                const common::uri::Uri& uri,
                                                player::media::AppOptions opt,
                                                player::media::ComplexOptions copt) {
  controller_->RequesRuntimeChannelInfo(sid);
  return base_class::CreateStream(sid, uri, opt, copt);
}

void Player::OnWindowCreated(SDL_Window* window, SDL_Renderer* render) {
  UNUSED(window);

  if (right_arrow_button_texture_) {
    chat_window_->SetShowButtonImage(right_arrow_button_texture_->GetTexture(render));
    plailist_window_->SetHideButtonImage(right_arrow_button_texture_->GetTexture(render));
  }
  if (left_arrow_button_texture_) {
    chat_window_->SetHideButtonImage(left_arrow_button_texture_->GetTexture(render));
    plailist_window_->SetShowButtonImage(left_arrow_button_texture_->GetTexture(render));
  }

  base_class::OnWindowCreated(window, render);
}

void Player::MoveToNextStream() {
  player::media::VideoState* stream = CreateNextStream();
  SetStream(stream);
}

void Player::MoveToPreviousStream() {
  player::media::VideoState* stream = CreatePrevStream();
  SetStream(stream);
}

player::media::VideoState* Player::CreateNextStream() {
  CHECK(THREAD_MANAGER()->IsMainThread());
  if (play_list_.empty()) {
    return nullptr;
  }

  size_t pos = GenerateNextPosition();
  player::media::VideoState* stream = CreateStreamPos(pos);
  return stream;
}

player::media::VideoState* Player::CreatePrevStream() {
  CHECK(THREAD_MANAGER()->IsMainThread());
  if (play_list_.empty()) {
    return nullptr;
  }

  size_t pos = GeneratePrevPosition();
  player::media::VideoState* stream = CreateStreamPos(pos);
  return stream;
}

player::media::VideoState* Player::CreateStreamPos(size_t pos) {
  CHECK(THREAD_MANAGER()->IsMainThread());
  current_stream_pos_ = pos;

  PlaylistEntry entry = play_list_[current_stream_pos_];
  ChannelInfo url = entry.GetChannelInfo();
  stream_id sid = url.GetId();
  player::media::AppOptions copy = GetStreamOptions();
  copy.enable_audio = url.IsEnableVideo();
  copy.enable_video = url.IsEnableAudio();

  plailist_window_->SetCurrentPositionInPlaylist(current_stream_pos_);
  player::media::VideoState* stream = CreateStream(sid, url.GetUrl(), copy, copt_);
  return stream;
}

size_t Player::GenerateNextPosition() const {
  if (current_stream_pos_ + 1 == play_list_.size()) {
    return 0;
  }

  return current_stream_pos_ + 1;
}

size_t Player::GeneratePrevPosition() const {
  if (current_stream_pos_ == 0) {
    return play_list_.size() - 1;
  }

  return current_stream_pos_ - 1;
}

void Player::StartShowFooter() {
  description_label_->SetVisible(true);
  player::media::msec_t cur_time = player::media::GetCurrentMsec();
  footer_last_shown_ = cur_time;
}

}  // namespace client
}  // namespace fastotv
