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

#include "client/playlist_entry.h"

#include <common/file_system.h>

#define IMG_UNKNOWN_CHANNEL_PATH_RELATIVE "share/resources/unknown_channel.png"

#define ICON_FILE_NAME "icon"

namespace fastotv {
namespace client {

PlaylistEntry::PlaylistEntry() : info_(), icon_(), cache_dir_() {}

PlaylistEntry::PlaylistEntry(const std::string& cache_root_dir, const ChannelInfo& info)
    : info_(info), rinfo_(), icon_(), cache_dir_() {
  std::string id = info_.GetId();
  cache_dir_ = common::file_system::make_path(cache_root_dir, id);
}

std::string PlaylistEntry::GetCacheDir() const {
  return cache_dir_;
}

std::string PlaylistEntry::GetIconPath() const {
  EpgInfo epg = info_.GetEpg();
  common::uri::Uri uri = epg.GetIconUrl();
  bool is_unknown_icon = EpgInfo::IsUnknownIconUrl(uri);
  if (is_unknown_icon) {
    const std::string absolute_source_dir = common::file_system::absolute_path_from_relative(RELATIVE_SOURCE_DIR);
    return common::file_system::make_path(absolute_source_dir, IMG_UNKNOWN_CHANNEL_PATH_RELATIVE);
  }
  std::string dir = GetCacheDir();
  return common::file_system::make_path(dir, ICON_FILE_NAME);
}

ChannelDescription PlaylistEntry::GetChannelDescription() const {
  std::string decr = "N/A";
  ChannelInfo url = GetChannelInfo();
  EpgInfo epg = url.GetEpg();
  ProgrammeInfo prog;
  if (epg.FindProgrammeByTime(common::time::current_mstime(), &prog)) {
    decr = prog.GetTitle();
  }

  return {url.GetName(), decr, GetIcon()};
}

void PlaylistEntry::SetIcon(channel_icon_t icon) {
  icon_ = icon;
}

channel_icon_t PlaylistEntry::GetIcon() const {
  return icon_;
}

ChannelInfo PlaylistEntry::GetChannelInfo() const {
  return info_;
}

void PlaylistEntry::SetRuntimeChannelInfo(const RuntimeChannelInfo& rinfo) {
  rinfo_ = rinfo;
}

RuntimeChannelInfo PlaylistEntry::GetRuntimeChannelInfo() const {
  return rinfo_;
}

}  // namespace client
}  // namespace fastotv
