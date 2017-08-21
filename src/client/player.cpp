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

#include <common/convert2string.h>
#include <common/file_system.h>
#include <common/threads/thread_manager.h>
#include <common/utils.h>

#include "client/ioservice.h"  // for IoService

#include "client/core/application/sdl2_application.h"

#include "client/sdl_utils.h"  // for IMG_LoadPNG, SurfaceSaver
#include "client/utils.h"

#define IMG_OFFLINE_CHANNEL_PATH_RELATIVE "share/resources/offline_channel.png"
#define IMG_CONNECTION_ERROR_PATH_RELATIVE "share/resources/connection_error.png"

#define CACHE_FOLDER_NAME "cache"

#define FOOTER_HIDE_DELAY_MSEC 2000  // 2 sec
#define KEYPAD_HIDE_DELAY_MSEC 3000  // 3 sec

namespace fasto {
namespace fastotv {
namespace client {

Player::Player(const std::string& app_directory_absolute_path,
               const PlayerOptions& options,
               const core::AppOptions& opt,
               const core::ComplexOptions& copt)
    : ISimplePlayer(options),
      offline_channel_texture_(nullptr),
      connection_error_texture_(nullptr),
      controller_(new IoService),
      current_stream_pos_(0),
      play_list_(),
      show_footer_(false),
      footer_last_shown_(0),
      current_state_str_("Init"),
      opt_(opt),
      copt_(copt),
      app_directory_absolute_path_(app_directory_absolute_path),
      show_keypad_(false),
      keypad_last_shown_(0),
      keypad_sym_(),
      show_programms_list_(true),
      last_programms_line_(0) {
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

  core::msec_t diff_keypad = cur_time - keypad_last_shown_;
  if (show_keypad_ && diff_keypad > KEYPAD_HIDE_DELAY_MSEC) {
    ResetKeyPad();
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

core::AppOptions Player::GetStreamOptions() const {
  return opt_;
}

const std::string& Player::GetAppDirectoryAbsolutePath() const {
  return app_directory_absolute_path_;
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
    const char* channel_icon_img_full_path_ptr = common::utils::c_strornull(icon_path);
    SDL_Surface* surface = IMG_Load(channel_icon_img_full_path_ptr);
    channel_icon_t shared_surface = std::make_shared<SurfaceSaver>(surface);
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

  SwitchToPlayingMode();
}

void Player::HandleWindowResizeEvent(core::events::WindowResizeEvent* event) {
  last_programms_line_ = 0;
  base_class::HandleWindowResizeEvent(event);
}

void Player::HandleWindowExposeEvent(core::events::WindowExposeEvent* event) {
  last_programms_line_ = 0;
  base_class::HandleWindowExposeEvent(event);
}

void Player::HandleKeyPressEvent(core::events::KeyPressEvent* event) {
  PlayerOptions opt = GetOptions();
  if (opt.exit_on_keydown) {
    return ISimplePlayer::HandleKeyPressEvent(event);
  }

  const core::events::KeyPressInfo inf = event->info();
  const SDL_Scancode scan_code = inf.ks.scancode;
  const Uint32 modifier = inf.ks.mod;
  if (scan_code == SDL_SCANCODE_KP_0) {
  } else if (scan_code == SDL_SCANCODE_KP_0) {
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
  } else if (scan_code == SDL_SCANCODE_RIGHT) {
    MoveToPreviousProgrammsPage();
  } else if (scan_code == SDL_SCANCODE_LEFT) {
    MoveToNextProgrammsPage();
  } else if (scan_code == SDL_SCANCODE_UP) {
    MoveToPreviousStream();
  } else if (scan_code == SDL_SCANCODE_DOWN) {
    MoveToNextStream();
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
  DrawKeyPad();
  DrawProgramsList();
}

SDL_Rect Player::GetFooterRect() const {
  const SDL_Rect display_rect = GetDrawRect();
  return {display_rect.x, display_rect.h - footer_height - volume_height - space_height + display_rect.y,
          display_rect.w, footer_height};
}

void Player::ToggleShowProgramsList() {
  show_programms_list_ = !show_programms_list_;
}

SDL_Rect Player::GetProgramsListRect() const {
  const SDL_Rect display_rect = GetDisplayRect();
  return {display_rect.x + display_rect.w * 3 / 4, display_rect.y, display_rect.w / 4, display_rect.h};
}

void Player::MoveToNextProgrammsPage() {
  if (play_list_.empty()) {
    last_programms_line_ = 0;
    return;
  }

  int lines = GetMaxProgrammsLines();
  int last_pos = play_list_.size() - 1;
  int max_size_on_page = last_pos - lines;
  if (max_size_on_page < 0) {
    last_programms_line_ = 0;
    return;
  }

  last_programms_line_ += lines;
  if (last_programms_line_ > last_pos) {
    last_programms_line_ = last_pos;
  }
}

void Player::MoveToPreviousProgrammsPage() {
  if (play_list_.empty()) {
    last_programms_line_ = 0;
    return;
  }

  int lines = GetMaxProgrammsLines();
  last_programms_line_ -= lines;
  if (last_programms_line_ < 0) {
    last_programms_line_ = 0;
  }
}

bool Player::GetChannelDescription(size_t pos, ChannelDescription* descr) const {
  if (!descr || pos >= play_list_.size()) {
    DNOTREACHED();
    return false;
  }

  PlaylistEntry entry = play_list_[pos];
  std::string decr = "N/A";
  ChannelInfo url = entry.GetChannelInfo();
  EpgInfo epg = url.GetEpg();
  ProgrammeInfo prog;
  if (epg.FindProgrammeByTime(common::time::current_mstime(), &prog)) {
    decr = prog.GetTitle();
  }

  *descr = {pos, url.GetName(), decr, entry.GetIcon()};
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

  show_keypad_ = true;
  core::msec_t cur_time = core::GetCurrentMsec();
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

  size_t stabled_pos = pos - 1;  // because start displaying channels from 1 in programms list
  if (stabled_pos >= play_list_.size()) {
    // error
    return;
  }

  core::VideoState* stream = CreateStreamPos(stabled_pos);
  SetStream(stream);
}

void Player::ResetKeyPad() {
  show_keypad_ = false;
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
  int font_height_2line = CalcHeightFontPlaceByRowCount(font, 2);
  return programms_list_rect.h / font_height_2line;
}

void Player::DrawProgramsList() {
  SDL_Renderer* render = GetRenderer();
  TTF_Font* font = GetFont();
  if (!show_programms_list_ || !font || !render || play_list_.empty()) {
    return;
  }

  const SDL_Rect programms_list_rect = GetProgramsListRect();
  int font_height_2line = CalcHeightFontPlaceByRowCount(font, 2);
  int min_size = keypad_width + font_height_2line + font_height_2line;  // number + icon + text
  if (programms_list_rect.w < min_size) {
    return;
  }

  int max_line_count = GetMaxProgrammsLines();
  if (max_line_count == 0) {
    return;
  }

  SDL_SetRenderDrawColor(render, 98, 118, 217, Uint8(SDL_ALPHA_OPAQUE * 0.5));
  SDL_RenderFillRect(render, &programms_list_rect);

  int drawed = 0;
  for (size_t i = last_programms_line_; i < play_list_.size() && drawed < max_line_count; ++i) {
    ChannelDescription descr;
    if (GetChannelDescription(i, &descr)) {
      int shift = 0;
      SDL_Rect cell_rect = {programms_list_rect.x, programms_list_rect.y + font_height_2line * drawed,
                            programms_list_rect.w, font_height_2line};
      SDL_Rect number_rect = {programms_list_rect.x, programms_list_rect.y + font_height_2line * drawed, keypad_width,
                              keypad_height};
      std::string number_str = common::ConvertToString(i + 1);
      DrawCenterTextInRect(number_str, text_color, number_rect);

      channel_icon_t icon = descr.icon;
      shift = keypad_width;  // in any case shift should be
      if (icon) {
        SDL_Texture* img = icon->GetTexture(render);
        if (img) {
          SDL_Rect icon_rect = {programms_list_rect.x + shift, programms_list_rect.y + font_height_2line * drawed,
                                font_height_2line, font_height_2line};
          SDL_RenderCopy(render, img, NULL, &icon_rect);
        }
      }
      shift += font_height_2line;  // in any case shift should be

      int text_width = programms_list_rect.w - shift;
      std::string title_line = DotText(common::MemSPrintf(" Title: %s", descr.title), font, text_width);
      std::string description_line =
          DotText(common::MemSPrintf(" Description: %s", descr.description), font, text_width);

      std::string line_text = common::MemSPrintf(
          "%s\n"
          "%s",
          title_line, description_line);
      SDL_Rect text_rect = {programms_list_rect.x + shift, programms_list_rect.y + font_height_2line * drawed,
                            text_width, font_height_2line};
      DrawWrappedTextInRect(line_text, text_color, text_rect);
      if (current_stream_pos_ == i) {  // seleceted item
        SDL_SetRenderDrawColor(render, 193, 66, 66, Uint8(SDL_ALPHA_OPAQUE * 0.5));
        SDL_RenderFillRect(render, &cell_rect);
      }
      drawed++;
    }
  }
}

void Player::DrawKeyPad() {
  SDL_Renderer* render = GetRenderer();
  TTF_Font* font = GetFont();
  if (!show_keypad_ || !font || !render) {
    return;
  }

  const char* keypad_sym_ptr = common::utils::c_strornull(keypad_sym_);
  if (!keypad_sym_ptr) {
    return;
  }
  const SDL_Rect keypad_rect = GetKeyPadRect();
  SDL_SetRenderDrawColor(render, 171, 217, 98, Uint8(SDL_ALPHA_OPAQUE * 0.5));
  SDL_RenderFillRect(render, &keypad_rect);
  DrawCenterTextInRect(keypad_sym_ptr, text_color, keypad_rect);
}

void Player::DrawFooter() {
  SDL_Renderer* render = GetRenderer();
  TTF_Font* font = GetFont();
  if (!show_footer_ || !font || !render) {
    return;
  }

  const SDL_Rect footer_rect = GetFooterRect();
  int padding_left = footer_rect.w / 4;
  SDL_Rect sdl_footer_rect = {footer_rect.x + padding_left, footer_rect.y, footer_rect.w - padding_left * 2,
                              footer_rect.h};
  States current_state = GetCurrentState();
  if (current_state == INIT_STATE) {
    std::string footer_text = current_state_str_;
    SDL_SetRenderDrawColor(render, 193, 66, 66, Uint8(SDL_ALPHA_OPAQUE * 0.5));
    SDL_RenderFillRect(render, &sdl_footer_rect);
    DrawCenterTextInRect(footer_text, text_color, sdl_footer_rect);
  } else if (current_state == FAILED_STATE) {
    std::string footer_text = current_state_str_;
    SDL_SetRenderDrawColor(render, 193, 66, 66, Uint8(SDL_ALPHA_OPAQUE * 0.5));
    SDL_RenderFillRect(render, &sdl_footer_rect);
    DrawCenterTextInRect(footer_text, text_color, sdl_footer_rect);
  } else if (current_state == PLAYING_STATE) {
    ChannelDescription descr;
    if (GetChannelDescription(current_stream_pos_, &descr)) {
      SDL_SetRenderDrawColor(render, 98, 118, 217, Uint8(SDL_ALPHA_OPAQUE * 0.5));
      SDL_RenderFillRect(render, &sdl_footer_rect);

      std::string footer_text = common::MemSPrintf(
          " Title: %s\n"
          " Description: %s",
          descr.title, descr.description);
      int h = CalcHeightFontPlaceByRowCount(font, 2);
      if (h > footer_rect.h) {
        h = footer_rect.h;
      }

      channel_icon_t icon = descr.icon;
      int shift = 0;
      if (icon) {
        SDL_Texture* img = icon->GetTexture(render);
        if (img) {
          SDL_Rect icon_rect = {sdl_footer_rect.x, sdl_footer_rect.y, h, h};
          SDL_RenderCopy(render, img, NULL, &icon_rect);
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
  SDL_Renderer* render = GetRenderer();
  if (!render) {
    return;
  }

  SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
  SDL_RenderClear(render);
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
  current_state_str_ = title;
  StartShowFooter();

  base_class::InitWindow(title, status);
}

void Player::DrawInitStatus() {
  SDL_Renderer* render = GetRenderer();
  if (!render) {
    return;
  }

  SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
  SDL_RenderClear(render);
  if (connection_error_texture_) {
    SDL_Texture* img = connection_error_texture_->GetTexture(render);
    if (img) {
      SDL_RenderCopy(render, img, NULL, NULL);
    }
  }
  DrawInfo();
  SDL_RenderPresent(render);
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

  PlaylistEntry entry = play_list_[current_stream_pos_];
  ChannelInfo url = entry.GetChannelInfo();
  stream_id sid = url.GetId();
  core::AppOptions copy = GetStreamOptions();
  copy.enable_audio = url.IsEnableVideo();
  copy.enable_video = url.IsEnableAudio();

  core::VideoState* stream = CreateStream(sid, url.GetUrl(), copy, copt_);
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
