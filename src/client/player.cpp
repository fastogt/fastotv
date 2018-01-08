/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

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
#include <common/file_system/file.h>
#include <common/file_system/file_system.h>
#include <common/file_system/string_path_utils.h>
#include <common/threads/thread_manager.h>
#include <common/utils.h>

#include <player/draw/surface_saver.h>
#include <player/sdl_utils.h>  // for IMG_LoadPNG, SurfaceSaver

// widgets
#include <player/draw/draw.h>
#include <player/gui/widgets/button.h>
#include <player/gui/widgets/icon_label.h>

#include "client/ioservice.h"  // for IoService
#include "client/utils.h"

#include "client/chat_window.h"
#include "client/programs_window.h"

#define IMG_OFFLINE_CHANNEL_PATH_RELATIVE "share/resources/offline_channel.png"
#define IMG_CONNECTION_ERROR_PATH_RELATIVE "share/resources/connection_error.png"

#define IMG_RIGHT_BUTTON_PATH_RELATIVE "share/resources/right_arrow.png"
#define IMG_LEFT_BUTTON_PATH_RELATIVE "share/resources/left_arrow.png"
#define IMG_UP_BUTTON_PATH_RELATIVE "share/resources/up_arrow.png"
#define IMG_DOWN_BUTTON_PATH_RELATIVE "share/resources/down_arrow.png"

#define CACHE_FOLDER_NAME "cache"

#define FOOTER_HIDE_DELAY_MSEC 2000  // 2 sec
#define KEYPAD_HIDE_DELAY_MSEC 3000  // 3 sec

namespace fastotv {
namespace client {

namespace {

fastoplayer::draw::SurfaceSaver* MakeSurfaceFromImageRelativePath(const std::string& relative_path) {
  const std::string absolute_source_dir = common::file_system::absolute_path_from_relative(RELATIVE_SOURCE_DIR);
  const std::string img_full_path = common::file_system::make_path(absolute_source_dir, relative_path);
  return fastoplayer::draw::MakeSurfaceFromPath(img_full_path);
}

}  // namespace

const SDL_Color Player::failed_color = {193, 66, 66, Uint8(SDL_ALPHA_OPAQUE * 0.5)};
const SDL_Color Player::playlist_color = {98, 118, 217, Uint8(SDL_ALPHA_OPAQUE * 0.5)};
const SDL_Color Player::keypad_color = fastoplayer::ISimplePlayer::stream_statistic_color;
const SDL_Color Player::playlist_item_preselect_color = {193, 66, 66, Uint8(SDL_ALPHA_OPAQUE * 0.1)};
const SDL_Color Player::chat_color = fastoplayer::ISimplePlayer::stream_statistic_color;
const SDL_Color Player::info_channel_color = {98, 118, 217, Uint8(SDL_ALPHA_OPAQUE * 0.5)};

Player::Player(const std::string& app_directory_absolute_path,
               const fastoplayer::PlayerOptions& options,
               const fastoplayer::media::AppOptions& opt,
               const fastoplayer::media::ComplexOptions& copt)
    : ISimplePlayer(options, RELATIVE_SOURCE_DIR),
      offline_channel_texture_(nullptr),
      connection_error_texture_(nullptr),
      right_arrow_button_texture_(nullptr),
      left_arrow_button_texture_(nullptr),
      up_arrow_button_texture_(nullptr),
      down_arrow_button_texture_(nullptr),
      show_playlist_button_(nullptr),
      hide_playlist_button_(nullptr),
      show_chat_button_(nullptr),
      hide_chat_button_(nullptr),
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
      programs_window_(nullptr),
      chat_window_(nullptr),
      auth_() {
  fApp->Subscribe(this, events::BandwidthEstimationEvent::EventType);

  fApp->Subscribe(this, events::ClientDisconnectedEvent::EventType);
  fApp->Subscribe(this, events::ClientConnectedEvent::EventType);

  fApp->Subscribe(this, events::ClientAuthorizedEvent::EventType);
  fApp->Subscribe(this, events::ClientUnAuthorizedEvent::EventType);

  fApp->Subscribe(this, events::ClientConfigChangeEvent::EventType);
  fApp->Subscribe(this, events::ReceiveChannelsEvent::EventType);
  fApp->Subscribe(this, events::ReceiveRuntimeChannelEvent::EventType);
  fApp->Subscribe(this, events::SendChatMessageEvent::EventType);
  fApp->Subscribe(this, events::ReceiveChatMessageEvent::EventType);

  // chat window
  chat_window_ = new ChatWindow(chat_color);
  chat_window_->SetTextColor(text_color);
  auto post_clicked_cb = [this](Uint8 button, const SDL_Point& position) {
    UNUSED(position);
    if (button == SDL_BUTTON_LEFT && auth_.IsValid()) {
      PlaylistEntry url;
      if (GetCurrentUrl(&url)) {
        ChannelInfo ch = url.GetChannelInfo();
        std::string text = chat_window_->GetInputText();
        if (!text.empty()) {
          chat_window_->ClearInputText();
          ChatMessage msg(ch.GetId(), auth_.GetLogin(), text, ChatMessage::MESSAGE);
          controller_->PostMessageToChat(msg);
        }
      }
    }
  };
  chat_window_->SetPostClickedCallback(post_clicked_cb);

  show_chat_button_ = new fastoplayer::gui::Button;
  auto show_chat_cb = [this](Uint8 button, const SDL_Point& position) {
    UNUSED(position);
    if (button == SDL_BUTTON_LEFT) {
      SetVisibleChat(true);
    }
  };
  show_chat_button_->SetMouseClickedCallback(show_chat_cb);
  show_chat_button_->SetTransparent(true);

  hide_chat_button_ = new fastoplayer::gui::Button;
  auto hide_chat_cb = [this](Uint8 button, const SDL_Point& position) {
    UNUSED(position);
    if (button == SDL_BUTTON_LEFT) {
      SetVisibleChat(false);
    }
  };
  hide_chat_button_->SetMouseClickedCallback(hide_chat_cb);
  hide_chat_button_->SetTransparent(true);
  SetVisibleChat(false);

  // descr window
  description_label_ = new fastoplayer::gui::IconLabel(failed_color);
  description_label_->SetTextColor(text_color);
  description_label_->SetSpace(space_width);
  description_label_->SetText("Init");

  // key_pad window
  keypad_label_ = new fastoplayer::gui::Label(keypad_color);
  keypad_label_->SetTextColor(text_color);
  keypad_label_->SetDrawType(fastoplayer::gui::Label::CENTER_TEXT);

  // playlist window
  programs_window_ = new ProgramsWindow(playlist_color);
  programs_window_->SetSelection(PlaylistWindow::SINGLE_ROW_SELECT);
  programs_window_->SetSelectionColor(playlist_item_preselect_color);
  programs_window_->SetDrawType(fastoplayer::gui::Label::WRAPPED_TEXT);
  programs_window_->SetTextColor(text_color);
  programs_window_->SetCurrentPositionSelectionColor(failed_color);
  auto channel_clicked_cb = [this](Uint8 button, size_t row) {
    if (button == SDL_BUTTON_LEFT) {
      if (row != current_stream_pos_) {
        fastoplayer::media::VideoState* stream = CreateStreamPos(row);
        SetStream(stream);
      }
    }
  };
  programs_window_->SetMouseClickedRowCallback(channel_clicked_cb);

  show_playlist_button_ = new fastoplayer::gui::Button;
  auto show_playlist_cb = [this](Uint8 button, const SDL_Point& position) {
    UNUSED(position);
    if (button == SDL_BUTTON_LEFT) {
      SetVisiblePlaylist(true);
    }
  };
  show_playlist_button_->SetMouseClickedCallback(show_playlist_cb);
  show_playlist_button_->SetTransparent(true);

  hide_playlist_button_ = new fastoplayer::gui::Button;
  auto hide_playlist_cb = [this](Uint8 button, const SDL_Point& position) {
    UNUSED(position);
    if (button == SDL_BUTTON_LEFT) {
      SetVisiblePlaylist(false);
    }
  };
  hide_playlist_button_->SetMouseClickedCallback(hide_playlist_cb);
  hide_playlist_button_->SetTransparent(true);
}

Player::~Player() {
  destroy(&show_playlist_button_);
  destroy(&hide_playlist_button_);
  destroy(&programs_window_);
  destroy(&keypad_label_);
  destroy(&description_label_);
  destroy(&show_chat_button_);
  destroy(&hide_chat_button_);
  destroy(&chat_window_);
  destroy(&controller_);
}

void Player::HandleEvent(event_t* event) {
  if (event->GetEventType() == events::BandwidthEstimationEvent::EventType) {
    events::BandwidthEstimationEvent* band_event = static_cast<events::BandwidthEstimationEvent*>(event);
    HandleBandwidthEstimationEvent(band_event);
  } else if (event->GetEventType() == events::ClientConnectedEvent::EventType) {
    events::ClientConnectedEvent* connect_event = static_cast<events::ClientConnectedEvent*>(event);
    HandleClientConnectedEvent(connect_event);
  } else if (event->GetEventType() == events::ClientDisconnectedEvent::EventType) {
    events::ClientDisconnectedEvent* disc_event = static_cast<events::ClientDisconnectedEvent*>(event);
    HandleClientDisconnectedEvent(disc_event);
  } else if (event->GetEventType() == events::ClientAuthorizedEvent::EventType) {
    events::ClientAuthorizedEvent* auth_event = static_cast<events::ClientAuthorizedEvent*>(event);
    HandleClientAuthorizedEvent(auth_event);
  } else if (event->GetEventType() == events::ClientUnAuthorizedEvent::EventType) {
    events::ClientUnAuthorizedEvent* unauth_event = static_cast<events::ClientUnAuthorizedEvent*>(event);
    HandleClientUnAuthorizedEvent(unauth_event);
  } else if (event->GetEventType() == events::ClientConfigChangeEvent::EventType) {
    events::ClientConfigChangeEvent* conf_change_event = static_cast<events::ClientConfigChangeEvent*>(event);
    HandleClientConfigChangeEvent(conf_change_event);
  } else if (event->GetEventType() == events::ReceiveChannelsEvent::EventType) {
    events::ReceiveChannelsEvent* channels_event = static_cast<events::ReceiveChannelsEvent*>(event);
    HandleReceiveChannelsEvent(channels_event);
  } else if (event->GetEventType() == events::ReceiveRuntimeChannelEvent::EventType) {
    events::ReceiveRuntimeChannelEvent* channel_event = static_cast<events::ReceiveRuntimeChannelEvent*>(event);
    HandleReceiveRuntimeChannelEvent(channel_event);
  } else if (event->GetEventType() == events::SendChatMessageEvent::EventType) {
    events::SendChatMessageEvent* chat_msg_event = static_cast<events::SendChatMessageEvent*>(event);
    HandleSendChatMessageEvent(chat_msg_event);
  } else if (event->GetEventType() == events::ReceiveChatMessageEvent::EventType) {
    events::ReceiveChatMessageEvent* chat_msg_event = static_cast<events::ReceiveChatMessageEvent*>(event);
    HandleReceiveChatMessageEvent(chat_msg_event);
  }

  base_class::HandleEvent(event);
}

void Player::HandleExceptionEvent(event_t* event, common::Error err) {
  if (event->GetEventType() == events::ClientConnectedEvent::EventType) {
    // gui::events::ClientConnectedEvent* connect_event =
    //    static_cast<gui::events::ClientConnectedEvent*>(event);
    SwitchToDisconnectMode();
  } else if (event->GetEventType() == events::ClientAuthorizedEvent::EventType) {
    // gui::events::ClientConnectedEvent* connect_event =
    //    static_cast<gui::events::ClientConnectedEvent*>(event);
    SwitchToUnAuthorizeMode();
  } else if (event->GetEventType() == events::BandwidthEstimationEvent::EventType) {
    events::BandwidthEstimationEvent* band_event = static_cast<events::BandwidthEstimationEvent*>(event);
    HandleBandwidthEstimationEvent(band_event);
  }

  base_class::HandleExceptionEvent(event, err);
}

void Player::HandlePreExecEvent(fastoplayer::gui::events::PreExecEvent* event) {
  fastoplayer::gui::events::PreExecInfo inf = event->GetInfo();
  if (inf.code == EXIT_SUCCESS) {
    offline_channel_texture_ = MakeSurfaceFromImageRelativePath(IMG_OFFLINE_CHANNEL_PATH_RELATIVE);
    connection_error_texture_ = MakeSurfaceFromImageRelativePath(IMG_CONNECTION_ERROR_PATH_RELATIVE);
    right_arrow_button_texture_ = MakeSurfaceFromImageRelativePath(IMG_RIGHT_BUTTON_PATH_RELATIVE);
    left_arrow_button_texture_ = MakeSurfaceFromImageRelativePath(IMG_LEFT_BUTTON_PATH_RELATIVE);
    up_arrow_button_texture_ = MakeSurfaceFromImageRelativePath(IMG_UP_BUTTON_PATH_RELATIVE);
    down_arrow_button_texture_ = MakeSurfaceFromImageRelativePath(IMG_DOWN_BUTTON_PATH_RELATIVE);
    controller_->Start();
    SwitchToConnectMode();
  }

  base_class::HandlePreExecEvent(event);
  TTF_Font* font = GetFont();
  int h = fastoplayer::draw::CalcHeightFontPlaceByRowCount(font, 2);

  int chat_row_height = h / 2;
  chat_window_->SetFont(font);
  chat_window_->SetRowHeight(chat_row_height);
  int cmin_size_width = ChatWindow::login_field_width + ChatWindow::space_width +
                        ChatWindow::login_field_width * 2;  // login + space + text
  common::draw::Size chat_minsize = {cmin_size_width, chat_row_height};
  chat_window_->SetMinimalSize(chat_minsize);

  const common::draw::Size icon_size(h, h);
  show_chat_button_->SetIconSize(icon_size);
  hide_chat_button_->SetIconSize(icon_size);

  description_label_->SetFont(font);
  keypad_label_->SetFont(font);
  programs_window_->SetFont(font);
  programs_window_->SetRowHeight(h);

  int pmin_size_width = keypad_width + h + space_width + h;  // number + icon + text
  common::draw::Size playlist_minsize = {pmin_size_width, h};
  programs_window_->SetMinimalSize(playlist_minsize);

  show_playlist_button_->SetIconSize(icon_size);
  hide_playlist_button_->SetIconSize(icon_size);
}

void Player::HandleTimerEvent(fastoplayer::gui::events::TimerEvent* event) {
  fastoplayer::media::msec_t cur_time = fastoplayer::media::GetCurrentMsec();
  fastoplayer::media::msec_t diff_footer = cur_time - footer_last_shown_;
  if (description_label_->IsVisible() && diff_footer > FOOTER_HIDE_DELAY_MSEC) {
    description_label_->SetVisible(false);
  }

  fastoplayer::media::msec_t diff_keypad = cur_time - keypad_last_shown_;
  if (keypad_label_->IsVisible() && diff_keypad > KEYPAD_HIDE_DELAY_MSEC) {
    ResetKeyPad();
  }

  base_class::HandleTimerEvent(event);
}

void Player::HandlePostExecEvent(fastoplayer::gui::events::PostExecEvent* event) {
  fastoplayer::gui::events::PostExecInfo inf = event->GetInfo();
  if (inf.code == EXIT_SUCCESS) {
    controller_->Stop();
    destroy(&offline_channel_texture_);
    destroy(&connection_error_texture_);
    destroy(&right_arrow_button_texture_);
    destroy(&left_arrow_button_texture_);
    destroy(&up_arrow_button_texture_);
    destroy(&down_arrow_button_texture_);
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

fastoplayer::media::AppOptions Player::GetStreamOptions() const {
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

  fastoplayer::PlayerOptions opt = GetOptions();

  size_t pos = current_stream_pos_;
  for (size_t i = 0; i < play_list_.size() && opt.last_showed_channel_id != invalid_stream_id; ++i) {
    PlaylistEntry ent = play_list_[i];
    ChannelInfo ch = ent.GetChannelInfo();
    if (ch.GetId() == opt.last_showed_channel_id) {
      pos = i;
      break;
    }
  }

  fastoplayer::media::VideoState* stream = CreateStreamPos(pos);
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

void Player::HandleBandwidthEstimationEvent(events::BandwidthEstimationEvent* event) {
  events::BandwidtInfo band_inf = event->GetInfo();
  if (band_inf.host_type == MAIN_SERVER) {
    controller_->RequestChannels();
  }
}

void Player::HandleClientConnectedEvent(events::ClientConnectedEvent* event) {
  UNUSED(event);
  SwitchToAuthorizeMode();
}

void Player::HandleClientDisconnectedEvent(events::ClientDisconnectedEvent* event) {
  UNUSED(event);
  if (GetCurrentState() == INIT_STATE) {
    SwitchToDisconnectMode();
  }
}

void Player::HandleClientAuthorizedEvent(events::ClientAuthorizedEvent* event) {
  auth_ = event->GetInfo();
  controller_->RequestServerInfo();
}

void Player::HandleClientUnAuthorizedEvent(events::ClientUnAuthorizedEvent* event) {
  UNUSED(event);
  auth_ = AuthInfo();
  if (GetCurrentState() == INIT_STATE) {
    SwitchToDisconnectMode();
  }
}

void Player::HandleClientConfigChangeEvent(events::ClientConfigChangeEvent* event) {
  UNUSED(event);
}

void Player::HandleReceiveChannelsEvent(events::ReceiveChannelsEvent* event) {
  ChannelsInfo chan = event->GetInfo();
  // prepare cache folders
  ChannelsInfo::channels_t channels = chan.GetChannels();
  const std::string cache_dir = common::file_system::make_path(app_directory_absolute_path_, CACHE_FOLDER_NAME);
  bool is_exist_cache_root = common::file_system::is_directory_exist(cache_dir);
  if (!is_exist_cache_root) {
    common::ErrnoError err = common::file_system::create_directory(cache_dir, true);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    } else {
      is_exist_cache_root = true;
    }
  }

  for (const ChannelInfo& ch : channels) {
    PlaylistEntry entry = PlaylistEntry(cache_dir, ch);
    const std::string icon_path = entry.GetIconPath();
    fastoplayer::draw::SurfaceSaver* surf = fastoplayer::draw::MakeSurfaceFromPath(icon_path);
    channel_icon_t shared_surface(surf);
    entry.SetIcon(shared_surface);
    play_list_.push_back(entry);

    if (is_exist_cache_root) {  // prepare cache folders for channels
      const std::string channel_dir = entry.GetCacheDir();
      bool is_cache_channel_dir_exist = common::file_system::is_directory_exist(channel_dir);
      if (!is_cache_channel_dir_exist) {
        common::ErrnoError err = common::file_system::create_directory(channel_dir, true);
        if (err) {
          DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
        } else {
          is_cache_channel_dir_exist = true;
        }
      }

      if (!is_cache_channel_dir_exist) {
        continue;
      }

      EpgInfo epg = ch.GetEpg();
      common::uri::Url uri = epg.GetIconUrl();
      bool is_unknown_icon = EpgInfo::IsUnknownIconUrl(uri);
      if (is_unknown_icon) {
        continue;
      }

      struct DownloadLimit {
        enum { timeout = 2 };
        DownloadLimit(IoService* controller) : controller_(controller), first_time_exec_(0) {}
        bool operator()() {
          if (!controller_->IsRunning()) {
            return true;
          }

          fastoplayer::media::msec_t cur_time = fastoplayer::media::GetCurrentMsec();
          if (first_time_exec_ == 0) {
            first_time_exec_ = cur_time;
          }
          fastoplayer::media::msec_t diff = cur_time - first_time_exec_;
          if (diff > timeout * 1000) {
            return true;
          }
          return false;
        }

       private:
        IoService* controller_;
        fastoplayer::media::msec_t first_time_exec_;
      };

      DownloadLimit download_interrupt_cb(controller_);
      auto load_image_cb = [download_interrupt_cb, entry, uri, channel_dir]() {
        const std::string channel_icon_path = entry.GetIconPath();
        if (!common::file_system::is_file_exist(channel_icon_path)) {  // if not exist trying to download
          common::buffer_t buff;
          bool is_file_downloaded = DownloadFileToBuffer(uri, &buff, download_interrupt_cb);
          if (!is_file_downloaded) {
            return;
          }

          const uint32_t fl = common::file_system::File::FLAG_CREATE | common::file_system::File::FLAG_WRITE |
                              common::file_system::File::FLAG_OPEN_BINARY;
          common::file_system::File channel_icon_file;
          common::ErrnoError err = channel_icon_file.Open(channel_icon_path, fl);
          if (err) {
            DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
            return;
          }

          size_t writed;
          err = channel_icon_file.Write(buff, &writed);
          if (err) {
            DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
            err = channel_icon_file.Close();
            return;
          }

          err = channel_icon_file.Close();
        }
      };
      controller_->ExecInLoopThread(load_image_cb);
    }
  }

  SetVisiblePlaylist(true);
  programs_window_->SetPlaylist(&play_list_);
  SwitchToPlayingMode();
}

void Player::HandleReceiveRuntimeChannelEvent(events::ReceiveRuntimeChannelEvent* event) {
  RuntimeChannelInfo inf = event->GetInfo();
  for (size_t i = 0; i < play_list_.size(); ++i) {
    ChannelInfo cinf = play_list_[i].GetChannelInfo();
    if (inf.GetChannelId() == cinf.GetId()) {
      play_list_[i].SetRuntimeChannelInfo(inf);
      break;
    }
  }
}

void Player::HandleSendChatMessageEvent(events::SendChatMessageEvent* event) {
  UNUSED(event);
}

void Player::HandleReceiveChatMessageEvent(events::ReceiveChatMessageEvent* event) {
  ChatMessage message = event->GetInfo();
  for (size_t i = 0; i < play_list_.size(); ++i) {
    ChannelInfo cinfo = play_list_[i].GetChannelInfo();
    if (cinfo.GetId() == message.GetChannelId()) {
      RuntimeChannelInfo rinfo = play_list_[i].GetRuntimeChannelInfo();
      rinfo.AddMessage(message);
      size_t watchers = rinfo.GetWatchersCount();
      if (message.GetType() == ChatMessage::CONTROL) {
        if (IsEnterMessage(message)) {
          rinfo.SetWatchersCount(watchers + 1);
        } else if (IsLeaveMessage(message)) {
          rinfo.SetWatchersCount(watchers - 1);
        }
      }
      chat_window_->SetMessages(rinfo.GetMessages());
      chat_window_->SetWatchers(rinfo.GetWatchersCount());
      play_list_[i].SetRuntimeChannelInfo(rinfo);
      break;
    }
  }
}

void Player::HandleKeyPressEvent(fastoplayer::gui::events::KeyPressEvent* event) {
  if (chat_window_->IsActived()) {
    return;
  }

  if (programs_window_->IsActived()) {
    return;
  }

  const fastoplayer::gui::events::KeyPressInfo inf = event->GetInfo();
  const SDL_Scancode scan_code = inf.ks.scancode;
  const Uint16 modifier = inf.ks.mod;
  bool is_acceptable_mods = modifier == KMOD_NONE || modifier & KMOD_NUM;
  if (scan_code == SDL_SCANCODE_KP_0 && modifier & KMOD_NUM) {
    HandleKeyPad(0);
  } else if (scan_code == SDL_SCANCODE_KP_1 && modifier & KMOD_NUM) {
    HandleKeyPad(1);
  } else if (scan_code == SDL_SCANCODE_KP_2 && modifier & KMOD_NUM) {
    HandleKeyPad(2);
  } else if (scan_code == SDL_SCANCODE_KP_3 && modifier & KMOD_NUM) {
    HandleKeyPad(3);
  } else if (scan_code == SDL_SCANCODE_KP_4 && modifier & KMOD_NUM) {
    HandleKeyPad(4);
  } else if (scan_code == SDL_SCANCODE_KP_5 && modifier & KMOD_NUM) {
    HandleKeyPad(5);
  } else if (scan_code == SDL_SCANCODE_KP_6 && modifier & KMOD_NUM) {
    HandleKeyPad(6);
  } else if (scan_code == SDL_SCANCODE_KP_7 && modifier & KMOD_NUM) {
    HandleKeyPad(7);
  } else if (scan_code == SDL_SCANCODE_KP_8 && modifier & KMOD_NUM) {
    HandleKeyPad(8);
  } else if (scan_code == SDL_SCANCODE_KP_9 && modifier & KMOD_NUM) {
    HandleKeyPad(9);
  } else if (scan_code == SDL_SCANCODE_BACKSPACE) {
    RemoveLastSymbolInKeypad();
  } else if (scan_code == SDL_SCANCODE_KP_ENTER && modifier & KMOD_NUM) {
    FinishKeyPadInput();
  } else if (scan_code == SDL_SCANCODE_F4) {
    StartShowFooter();
  } else if (scan_code == SDL_SCANCODE_F5) {
    ToggleShowProgramsList();
  } else if (scan_code == SDL_SCANCODE_F6) {
    ToggleShowChat();
  } else if (scan_code == SDL_SCANCODE_UP) {
    if (is_acceptable_mods) {
      MoveToPreviousStream();
    }
  } else if (scan_code == SDL_SCANCODE_DOWN) {
    if (is_acceptable_mods) {
      MoveToNextStream();
    }
  }

  base_class::HandleKeyPressEvent(event);
}

void Player::HandleLircPressEvent(fastoplayer::gui::events::LircPressEvent* event) {
  fastoplayer::gui::events::LircPressInfo inf = event->GetInfo();
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
  SetVisiblePlaylist(!programs_window_->IsVisible());
}

void Player::ToggleShowChat() {
  SetVisibleChat(!chat_window_->IsVisible());
}

SDL_Rect Player::GetProgramsListRect() const {
  const SDL_Rect display_rect = GetDisplayRect();
  return {display_rect.x + display_rect.w * 3 / 4, display_rect.y, display_rect.w / 4, display_rect.h};
}

SDL_Rect Player::GetChatRect() const {
  const SDL_Rect display_rect = GetDisplayRect();
  int height = display_rect.h / 4;
  int width = display_rect.w;
  if (programs_window_->IsVisible()) {
    width = display_rect.w * 3 / 4;
  }
  return {0, display_rect.h - height, width, height};
}

SDL_Rect Player::GetHideButtonPlayListRect() const {
  const common::draw::Size sz = hide_playlist_button_->GetIconSize();
  SDL_Rect prog_rect = GetProgramsListRect();
  SDL_Rect hide_button_rect = {prog_rect.x - sz.width, prog_rect.h / 2, sz.width, sz.height};
  return hide_button_rect;
}

SDL_Rect Player::GetShowButtonPlayListRect() const {
  const common::draw::Size sz = show_playlist_button_->GetIconSize();
  SDL_Rect prog_rect = GetProgramsListRect();
  SDL_Rect show_button_rect = {prog_rect.x + prog_rect.w - sz.width, prog_rect.h / 2, sz.width, sz.height};
  return show_button_rect;
}

SDL_Rect Player::GetHideButtonChatRect() const {
  const common::draw::Size sz = hide_chat_button_->GetIconSize();
  SDL_Rect chat_rect = GetChatRect();
  SDL_Rect show_button_rect = {chat_rect.w / 2, chat_rect.y - sz.height, sz.width, sz.height};
  return show_button_rect;
}

SDL_Rect Player::GetShowButtonChatRect() const {
  const common::draw::Size sz = show_chat_button_->GetIconSize();
  SDL_Rect chat_rect = GetChatRect();
  SDL_Rect hide_button_rect = {chat_rect.w / 2, chat_rect.y + chat_rect.h - sz.height, sz.width, sz.height};
  return hide_button_rect;
}

void Player::SetVisiblePlaylist(bool visible) {
  programs_window_->SetVisible(visible);
  hide_playlist_button_->SetVisible(visible);
  show_playlist_button_->SetVisible(!visible);
}

void Player::SetVisibleChat(bool visible) {
  chat_window_->SetVisible(visible);
  hide_chat_button_->SetVisible(visible);
  show_chat_button_->SetVisible(!visible);
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

  const std::string keypad_sym = keypad_label_->GetText();
  size_t cur_number;
  if (!common::ConvertFromString(keypad_sym, &cur_number)) {
    cur_number = 0;
  }

  keypad_label_->SetVisible(true);
  fastoplayer::media::msec_t cur_time = fastoplayer::media::GetCurrentMsec();
  keypad_last_shown_ = cur_time;
  size_t nex_keypad_sym = cur_number * 10 + key;
  if (nex_keypad_sym <= max_keypad_size) {
    keypad_label_->SetText(common::ConvertToString(nex_keypad_sym));
  }
}

void Player::RemoveLastSymbolInKeypad() {
  const std::string keypad_sym = keypad_label_->GetText();
  size_t cur_number;
  if (!common::ConvertFromString(keypad_sym, &cur_number)) {
    return;
  }

  size_t nex_keypad_sym = cur_number / 10;
  if (nex_keypad_sym == 0) {
    ResetKeyPad();
    return;
  }

  keypad_label_->SetText(common::ConvertToString(nex_keypad_sym));
}

void Player::FinishKeyPadInput() {
  const std::string keypad_sym = keypad_label_->GetText();
  size_t pos;
  if (!common::ConvertFromString(keypad_sym, &pos)) {
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

  fastoplayer::media::VideoState* stream = CreateStreamPos(stabled_pos);
  SetStream(stream);
}

void Player::ResetKeyPad() {
  keypad_label_->SetVisible(false);
  keypad_label_->ClearText();
}

SDL_Rect Player::GetKeyPadRect() const {
  const SDL_Rect display_rect = GetDrawRect();
  return {display_rect.w + space_width - keypad_width, display_rect.y, keypad_width, keypad_height};
}

void Player::DrawProgramsList() {
  SDL_Renderer* render = GetRenderer();
  TTF_Font* font = GetFont();
  if (!font || !render) {
    return;
  }

  const SDL_Rect programms_list_rect = GetProgramsListRect();
  programs_window_->SetRect(programms_list_rect);
  programs_window_->Draw(render);

  if (fApp->IsCursorVisible() && programs_window_->IsSizeEnough()) {
    hide_playlist_button_->SetRect(GetHideButtonPlayListRect());
    hide_playlist_button_->Draw(render);
    show_playlist_button_->SetRect(GetShowButtonPlayListRect());
    show_playlist_button_->Draw(render);
  }
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

  chat_window_->SetPostMessageEnabled(!rinfo.IsChatReadOnly());
  const SDL_Rect chat_rect = GetChatRect();
  chat_window_->SetRect(chat_rect);
  chat_window_->Draw(render);

  if (fApp->IsCursorVisible() && chat_window_->IsSizeEnough()) {
    hide_chat_button_->SetRect(GetHideButtonChatRect());
    hide_chat_button_->Draw(render);
    show_chat_button_->SetRect(GetShowButtonChatRect());
    show_chat_button_->Draw(render);
  }
}

void Player::DrawKeyPad() {
  SDL_Renderer* render = GetRenderer();
  TTF_Font* font = GetFont();
  if (!keypad_label_->IsVisible() || !font) {
    return;
  }

  const SDL_Rect keypad_rect = GetKeyPadRect();
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

  fastoplayer::draw::FlushRender(render, fastoplayer::draw::black_color);
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
    description_label_->SetDrawType(fastoplayer::gui::Label::CENTER_TEXT);
    description_label_->SetIconTexture(NULL);
    description_label_->SetBackGroundColor(failed_color);
  } else if (new_state == FAILED_STATE) {
    description_label_->SetDrawType(fastoplayer::gui::Label::CENTER_TEXT);
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
        int h = fastoplayer::draw::CalcHeightFontPlaceByRowCount(font, DESCR_LINES_COUNT);
        if (h > footer_rect.h) {
          h = footer_rect.h;
        }
        description_label_->SetIconSize(common::draw::Size(h, h));
      } else {
        description_label_->SetIconTexture(NULL);
      }
      description_label_->SetDrawType(fastoplayer::gui::Label::WRAPPED_TEXT);
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

  fastoplayer::draw::FlushRender(render, fastoplayer::draw::black_color);
  if (connection_error_texture_) {
    SDL_Texture* img = connection_error_texture_->GetTexture(render);
    if (img) {
      SDL_RenderCopy(render, img, NULL, NULL);
    }
  }
  DrawInfo();
  SDL_RenderPresent(render);
}

fastoplayer::media::VideoState* Player::CreateStream(stream_id sid,
                                                     const common::uri::Url& uri,
                                                     fastoplayer::media::AppOptions opt,
                                                     fastoplayer::media::ComplexOptions copt) {
  controller_->RequesRuntimeChannelInfo(sid);
  return base_class::CreateStream(sid, uri, opt, copt);
}

void Player::OnWindowCreated(SDL_Window* window, SDL_Renderer* render) {
  if (right_arrow_button_texture_) {
    hide_playlist_button_->SetIconTexture(right_arrow_button_texture_->GetTexture(render));
  }
  if (left_arrow_button_texture_) {
    show_playlist_button_->SetIconTexture(left_arrow_button_texture_->GetTexture(render));
  }
  if (up_arrow_button_texture_) {
    show_chat_button_->SetIconTexture(up_arrow_button_texture_->GetTexture(render));
  }
  if (down_arrow_button_texture_) {
    hide_chat_button_->SetIconTexture(down_arrow_button_texture_->GetTexture(render));
  }

  base_class::OnWindowCreated(window, render);
}

void Player::MoveToNextStream() {
  fastoplayer::media::VideoState* stream = CreateNextStream();
  SetStream(stream);
}

void Player::MoveToPreviousStream() {
  fastoplayer::media::VideoState* stream = CreatePrevStream();
  SetStream(stream);
}

fastoplayer::media::VideoState* Player::CreateNextStream() {
  CHECK(THREAD_MANAGER()->IsMainThread());
  if (play_list_.empty()) {
    return nullptr;
  }

  size_t pos = GenerateNextPosition();
  fastoplayer::media::VideoState* stream = CreateStreamPos(pos);
  return stream;
}

fastoplayer::media::VideoState* Player::CreatePrevStream() {
  CHECK(THREAD_MANAGER()->IsMainThread());
  if (play_list_.empty()) {
    return nullptr;
  }

  size_t pos = GeneratePrevPosition();
  fastoplayer::media::VideoState* stream = CreateStreamPos(pos);
  return stream;
}

fastoplayer::media::VideoState* Player::CreateStreamPos(size_t pos) {
  CHECK(THREAD_MANAGER()->IsMainThread());
  current_stream_pos_ = pos;

  PlaylistEntry entry = play_list_[current_stream_pos_];
  ChannelInfo url = entry.GetChannelInfo();
  stream_id sid = url.GetId();
  fastoplayer::media::AppOptions copy = GetStreamOptions();
  copy.enable_audio = url.IsEnableVideo();
  copy.enable_video = url.IsEnableAudio();

  programs_window_->SetCurrentPositionInPlaylist(current_stream_pos_);
  fastoplayer::media::VideoState* stream = CreateStream(sid, url.GetUrl(), copy, copt_);
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
  fastoplayer::media::msec_t cur_time = fastoplayer::media::GetCurrentMsec();
  footer_last_shown_ = cur_time;
}

}  // namespace client
}  // namespace fastotv
