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

#include <SDL2/SDL_image.h>

#include <common/file_system.h>
#include <common/utils.h>
#include <common/threads/thread_manager.h>

#include "client/ioservice.h"  // for IoService

#include "client/core/application/sdl2_application.h"

#include "client/sdl_utils.h"  // for IMG_LoadPNG, SurfaceSaver
#include "client/utils.h"

#define IMG_OFFLINE_CHANNEL_PATH_RELATIVE "share/resources/offline_channel.png"
#define IMG_CONNECTION_ERROR_PATH_RELATIVE "share/resources/connection_error.png"

#define CACHE_FOLDER_NAME "cache"

#define FOOTER_HIDE_DELAY_MSEC 2000  // 2 sec

namespace fasto {
namespace fastotv {
namespace client {

Player::Player(const std::string& app_directory_absolute_path,
               const PlayerOptions& options,
               const core::AppOptions& opt,
               const core::ComplexOptions& copt)
    : ISimplePlayer(app_directory_absolute_path, options, opt, copt),
      offline_channel_texture_(nullptr),
      connection_error_texture_(nullptr),
      controller_(new IoService),
      current_stream_pos_(0),
      play_list_(),
      show_footer_(false),
      footer_last_shown_(0),
      current_state_str_("Init") {
  fApp->Subscribe(this, core::events::BandwidthEstimationEvent::EventType);

  fApp->Subscribe(this, core::events::ClientDisconnectedEvent::EventType);
  fApp->Subscribe(this, core::events::ClientConnectedEvent::EventType);

  fApp->Subscribe(this, core::events::ClientAuthorizedEvent::EventType);
  fApp->Subscribe(this, core::events::ClientUnAuthorizedEvent::EventType);

  fApp->Subscribe(this, core::events::ClientConfigChangeEvent::EventType);
  fApp->Subscribe(this, core::events::ReceiveChannelsEvent::EventType);
}

Player::~Player() {
  destroy(&controller_);
}

void Player::HandleEvent(event_t* event) {
  if (event->GetEventType() == core::events::BandwidthEstimationEvent::EventType) {
    core::events::BandwidthEstimationEvent* band_event = static_cast<core::events::BandwidthEstimationEvent*>(event);
    HandleBandwidthEstimationEvent(band_event);
  } else if (event->GetEventType() == core::events::ClientConnectedEvent::EventType) {
    core::events::ClientConnectedEvent* connect_event = static_cast<core::events::ClientConnectedEvent*>(event);
    HandleClientConnectedEvent(connect_event);
  } else if (event->GetEventType() == core::events::ClientDisconnectedEvent::EventType) {
    core::events::ClientDisconnectedEvent* disc_event = static_cast<core::events::ClientDisconnectedEvent*>(event);
    HandleClientDisconnectedEvent(disc_event);
  } else if (event->GetEventType() == core::events::ClientAuthorizedEvent::EventType) {
    core::events::ClientAuthorizedEvent* auth_event = static_cast<core::events::ClientAuthorizedEvent*>(event);
    HandleClientAuthorizedEvent(auth_event);
  } else if (event->GetEventType() == core::events::ClientUnAuthorizedEvent::EventType) {
    core::events::ClientUnAuthorizedEvent* unauth_event = static_cast<core::events::ClientUnAuthorizedEvent*>(event);
    HandleClientUnAuthorizedEvent(unauth_event);
  } else if (event->GetEventType() == core::events::ClientConfigChangeEvent::EventType) {
    core::events::ClientConfigChangeEvent* conf_change_event =
        static_cast<core::events::ClientConfigChangeEvent*>(event);
    HandleClientConfigChangeEvent(conf_change_event);
  } else if (event->GetEventType() == core::events::ReceiveChannelsEvent::EventType) {
    core::events::ReceiveChannelsEvent* channels_event = static_cast<core::events::ReceiveChannelsEvent*>(event);
    HandleReceiveChannelsEvent(channels_event);
  }

  base_class::HandleEvent(event);
}

void Player::HandleExceptionEvent(event_t* event, common::Error err) {
  if (event->GetEventType() == core::events::ClientConnectedEvent::EventType) {
    // core::events::ClientConnectedEvent* connect_event =
    //    static_cast<core::events::ClientConnectedEvent*>(event);
    SwitchToDisconnectMode();
  } else if (event->GetEventType() == core::events::ClientAuthorizedEvent::EventType) {
    // core::events::ClientConnectedEvent* connect_event =
    //    static_cast<core::events::ClientConnectedEvent*>(event);
    SwitchToUnAuthorizeMode();
  } else if (event->GetEventType() == core::events::BandwidthEstimationEvent::EventType) {
    core::events::BandwidthEstimationEvent* band_event = static_cast<core::events::BandwidthEstimationEvent*>(event);
    HandleBandwidthEstimationEvent(band_event);
  }

  base_class::HandleExceptionEvent(event, err);
}

void Player::HandlePreExecEvent(core::events::PreExecEvent* event) {
  core::events::PreExecInfo inf = event->info();
  if (inf.code == EXIT_SUCCESS) {
    const std::string absolute_source_dir = common::file_system::absolute_path_from_relative(RELATIVE_SOURCE_DIR);
    const std::string offline_channel_img_full_path =
        common::file_system::make_path(absolute_source_dir, IMG_OFFLINE_CHANNEL_PATH_RELATIVE);
    const char* offline_channel_img_full_path_ptr = common::utils::c_strornull(offline_channel_img_full_path);
    SDL_Surface* surface = IMG_Load(offline_channel_img_full_path_ptr);
    if (surface) {
      offline_channel_texture_ = new SurfaceSaver(surface);
    }

    const std::string connection_error_img_full_path =
        common::file_system::make_path(absolute_source_dir, IMG_CONNECTION_ERROR_PATH_RELATIVE);
    const char* connection_error_img_full_path_ptr = common::utils::c_strornull(connection_error_img_full_path);
    SDL_Surface* surface2 = IMG_Load(connection_error_img_full_path_ptr);
    if (surface2) {
      connection_error_texture_ = new SurfaceSaver(surface2);
    }
    controller_->Start();
    SwitchToConnectMode();
  }
  base_class::HandlePreExecEvent(event);
}

void Player::HandleTimerEvent(core::events::TimerEvent* event) {
  core::msec_t cur_time = core::GetCurrentMsec();
  core::msec_t diff_footer = cur_time - footer_last_shown_;
  if (show_footer_ && diff_footer > FOOTER_HIDE_DELAY_MSEC) {
    show_footer_ = false;
  }

  base_class::HandleTimerEvent(event);
}

void Player::HandlePostExecEvent(core::events::PostExecEvent* event) {
  core::events::PostExecInfo inf = event->info();
  if (inf.code == EXIT_SUCCESS) {
    controller_->Stop();
    destroy(&offline_channel_texture_);
    destroy(&connection_error_texture_);
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

  PlayerOptions opt = GetOptions();

  size_t pos = current_stream_pos_;
  for (size_t i = 0; i < play_list_.size() && opt.last_showed_channel_id != invalid_stream_id; ++i) {
    PlaylistEntry ent = play_list_[i];
    ChannelInfo ch = ent.GetChannelInfo();
    if (ch.GetId() == opt.last_showed_channel_id) {
      pos = i;
      break;
    }
  }

  core::VideoState* stream = CreateStreamPos(pos);
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

void Player::HandleBandwidthEstimationEvent(core::events::BandwidthEstimationEvent* event) {
  core::events::BandwidtInfo band_inf = event->info();
  if (band_inf.host_type == MAIN_SERVER) {
    controller_->RequestChannels();
  }
}

void Player::HandleClientConnectedEvent(core::events::ClientConnectedEvent* event) {
  UNUSED(event);
  SwitchToAuthorizeMode();
}

void Player::HandleClientDisconnectedEvent(core::events::ClientDisconnectedEvent* event) {
  UNUSED(event);
  if (GetCurrentState() == INIT_STATE) {
    SwitchToDisconnectMode();
  }
}

void Player::HandleClientAuthorizedEvent(core::events::ClientAuthorizedEvent* event) {
  UNUSED(event);

  controller_->RequestServerInfo();
}

void Player::HandleClientUnAuthorizedEvent(core::events::ClientUnAuthorizedEvent* event) {
  UNUSED(event);
  if (GetCurrentState() == INIT_STATE) {
    SwitchToDisconnectMode();
  }
}

void Player::HandleClientConfigChangeEvent(core::events::ClientConfigChangeEvent* event) {
  UNUSED(event);
}

void Player::HandleReceiveChannelsEvent(core::events::ReceiveChannelsEvent* event) {
  ChannelsInfo chan = event->info();
  // prepare cache folders
  ChannelsInfo::channels_t channels = chan.GetChannels();
  const std::string cache_dir = common::file_system::make_path(GetAppDirectoryAbsolutePath(), CACHE_FOLDER_NAME);
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

  SwitchToPlayingMode();
}

void Player::HandleKeyPressEvent(core::events::KeyPressEvent* event) {
  PlayerOptions opt = GetOptions();
  if (opt.exit_on_keydown) {
    return ISimplePlayer::HandleKeyPressEvent(event);
  }

  core::events::KeyPressInfo inf = event->info();
  switch (inf.ks.sym) {
    case FASTO_KEY_LEFTBRACKET: {
      MoveToPreviousStream();
      break;
    }
    case FASTO_KEY_RIGHTBRACKET: {
      MoveToNextStream();
      break;
    }
    case FASTO_KEY_F4: {
      StartShowFooter();
      break;
    }
    default: { break; }
  }

  ISimplePlayer::HandleKeyPressEvent(event);
}

void Player::HandleLircPressEvent(core::events::LircPressEvent* event) {
  PlayerOptions opt = GetOptions();
  if (opt.exit_on_keydown) {
    return ISimplePlayer::HandleLircPressEvent(event);
  }

  core::events::LircPressInfo inf = event->info();
  switch (inf.code) {
    case LIRC_KEY_LEFT: {
      MoveToPreviousStream();
      break;
    }
    case LIRC_KEY_RIGHT: {
      MoveToNextStream();
      break;
    }
    default: { break; }
  }

  ISimplePlayer::HandleLircPressEvent(event);
}

void Player::DrawInfo() {
  DrawStatistic();
  DrawFooter();
  DrawVolume();
}

SDL_Rect Player::GetFooterRect() const {
  const SDL_Rect display_rect = GetDrawRect();
  return {display_rect.x, display_rect.h - footer_height - volume_height - space_height + display_rect.y,
          display_rect.w, footer_height};
}

void Player::DrawFooter() {
  if (!show_footer_ || !font_ || !renderer_) {
    return;
  }

  const SDL_Rect footer_rect = GetFooterRect();
  int padding_left = footer_rect.w / 4;
  SDL_Rect sdl_footer_rect = {footer_rect.x + padding_left, footer_rect.y, footer_rect.w - padding_left * 2,
                              footer_rect.h};
  States current_state = base_class::GetCurrentState();
  if (current_state == INIT_STATE) {
    std::string footer_text = current_state_str_;
    SDL_SetRenderDrawColor(renderer_, 193, 66, 66, Uint8(SDL_ALPHA_OPAQUE * 0.5));
    SDL_RenderFillRect(renderer_, &sdl_footer_rect);
    DrawCenterTextInRect(footer_text, text_color, sdl_footer_rect);
  } else if (current_state == FAILED_STATE) {
    std::string footer_text = current_state_str_;
    SDL_SetRenderDrawColor(renderer_, 193, 66, 66, Uint8(SDL_ALPHA_OPAQUE * 0.5));
    SDL_RenderFillRect(renderer_, &sdl_footer_rect);
    DrawCenterTextInRect(footer_text, text_color, sdl_footer_rect);
  } else if (current_state == PLAYING_STATE) {
    PlaylistEntry entry;
    if (GetCurrentUrl(&entry)) {
      std::string decr = "N/A";
      ChannelInfo url = entry.GetChannelInfo();
      EpgInfo epg = url.GetEpg();
      ProgrammeInfo prog;
      if (epg.FindProgrammeByTime(common::time::current_mstime(), &prog)) {
        decr = prog.GetTitle();
      }

      SDL_SetRenderDrawColor(renderer_, 98, 118, 217, Uint8(SDL_ALPHA_OPAQUE * 0.5));
      SDL_RenderFillRect(renderer_, &sdl_footer_rect);

      std::string footer_text = common::MemSPrintf(
          " Title: %s\n"
          " Description: %s",
          url.GetName(), decr);
      int h = CalcHeightFontPlaceByRowCount(font_, 2);
      if (h > footer_rect.h) {
        h = footer_rect.h;
      }

      auto icon = entry.GetIcon();
      int shift = 0;
      if (icon) {
        SDL_Texture* img = icon->GetTexture(renderer_);
        if (img) {
          SDL_Rect icon_rect = {sdl_footer_rect.x, sdl_footer_rect.y, h, h};
          SDL_RenderCopy(renderer_, img, NULL, &icon_rect);
          shift = h;
        }
      }

      SDL_Rect text_rect = {sdl_footer_rect.x + shift, sdl_footer_rect.y, sdl_footer_rect.w - shift, h};
      DrawWrappedTextInRect(footer_text, text_color, text_rect);
    }
  } else {
    NOTREACHED();
  }
}

void Player::DrawFailedStatus() {
  if (!renderer_) {
    return;
  }

  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
  SDL_RenderClear(renderer_);
  if (offline_channel_texture_) {
    SDL_Texture* img = offline_channel_texture_->GetTexture(renderer_);
    if (img) {
      SDL_RenderCopy(renderer_, img, NULL, NULL);
    }
  }
  DrawInfo();
  SDL_RenderPresent(renderer_);
}

void Player::InitWindow(const std::string& title, States status) {
  current_state_str_ = title;
  StartShowFooter();

  base_class::InitWindow(title, status);
}

void Player::DrawInitStatus() {
  if (!renderer_) {
    return;
  }

  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
  SDL_RenderClear(renderer_);
  if (connection_error_texture_) {
    SDL_Texture* img = connection_error_texture_->GetTexture(renderer_);
    if (img) {
      SDL_RenderCopy(renderer_, img, NULL, NULL);
    }
  }
  DrawInfo();
  SDL_RenderPresent(renderer_);
}

void Player::MoveToNextStream() {
  core::VideoState* stream = CreateNextStream();
  SetStream(stream);
}

void Player::MoveToPreviousStream() {
  core::VideoState* stream = CreatePrevStream();
  SetStream(stream);
}

core::VideoState* Player::CreateNextStream() {
  CHECK(THREAD_MANAGER()->IsMainThread());
  if (play_list_.empty()) {
    return nullptr;
  }

  size_t pos = GenerateNextPosition();
  core::VideoState* stream = CreateStreamPos(pos);
  return stream;
}

core::VideoState* Player::CreatePrevStream() {
  CHECK(THREAD_MANAGER()->IsMainThread());
  if (play_list_.empty()) {
    return nullptr;
  }

  size_t pos = GeneratePrevPosition();
  core::VideoState* stream = CreateStreamPos(pos);
  return stream;
}

core::VideoState* Player::CreateStreamPos(size_t pos) {
  CHECK(THREAD_MANAGER()->IsMainThread());
  current_stream_pos_ = pos;

  PlaylistEntry entr = play_list_[current_stream_pos_];
  if (!entr.GetIcon()) {  // try to upload image
    const std::string icon_path = entr.GetIconPath();
    const char* channel_icon_img_full_path_ptr = common::utils::c_strornull(icon_path);
    SDL_Surface* surface = IMG_Load(channel_icon_img_full_path_ptr);
    channel_icon_t shared_surface = common::make_shared<SurfaceSaver>(surface);
    play_list_[current_stream_pos_].SetIcon(shared_surface);
  }

  ChannelInfo url = entr.GetChannelInfo();
  stream_id sid = url.GetId();
  core::AppOptions copy = GetStreamOptions();
  copy.enable_audio = url.IsEnableVideo();
  copy.enable_video = url.IsEnableAudio();

  core::VideoState* stream = CreateStream(sid, url.GetUrl(), copy);
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
  show_footer_ = true;
  core::msec_t cur_time = core::GetCurrentMsec();
  footer_last_shown_ = cur_time;
}

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
