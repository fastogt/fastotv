/*  Copyright (C) 2014-2020 FastoGT. All right reserved.

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

#include <memory>
#include <string>

#include <fastotv/commands_info/runtime_channel_info.h>
#include <fastotv/commands_info/vods_info.h>

namespace fastoplayer {
namespace draw {
class SurfaceSaver;
}
}  // namespace fastoplayer

namespace fastotv {
namespace client {

typedef std::shared_ptr<fastoplayer::draw::SurfaceSaver> channel_icon_t;

struct VodDescription {
  std::string title;
  std::string description;
  channel_icon_t icon;
};

class VodEntry {
 public:
  VodEntry();
  VodEntry(const std::string& cache_root_dir, const commands_info::VodInfo& info);

  commands_info::VodInfo GetVodInfo() const;

  void SetRuntimeChannelInfo(const commands_info::RuntimeChannelInfo& rinfo);
  commands_info::RuntimeChannelInfo GetRuntimeChannelInfo() const;

  void SetIcon(channel_icon_t icon);
  channel_icon_t GetIcon() const;

  std::string GetCacheDir() const;
  std::string GetIconPath() const;

  VodDescription GetChannelDescription() const;

 private:
  commands_info::VodInfo info_;
  commands_info::RuntimeChannelInfo rinfo_;

  channel_icon_t icon_;
  std::string cache_dir_;
};

}  // namespace client
}  // namespace fastotv
