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

#include <common/smart_ptr.h>

#include "channels_info.h"

namespace fasto {
namespace fastotv {
namespace client {
class TextureSaver;
}
}  // namespace fastotv
}  // namespace fasto

namespace fasto {
namespace fastotv {
namespace client {

typedef common::shared_ptr<TextureSaver> channel_icon_t;

class PlaylistEntry {
 public:
  PlaylistEntry();
  PlaylistEntry(const std::string& cache_root_dir, const ChannelInfo& info);

  ChannelInfo GetChannelInfo() const;

  void SetIcon(channel_icon_t icon);
  channel_icon_t GetIcon() const;

  std::string GetCacheDir() const;
  std::string GetIconPath() const;

 private:
  ChannelInfo info_;
  channel_icon_t icon_;
  std::string cache_dir_;
};

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
