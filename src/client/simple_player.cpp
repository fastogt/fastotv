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

#include "client/simple_player.h"

namespace fasto {
namespace fastotv {
namespace client {

SimplePlayer::SimplePlayer(const common::uri::Uri& stream_url,
                           const std::string& app_directory_absolute_path,
                           const PlayerOptions& options,
                           const core::AppOptions& opt,
                           const core::ComplexOptions& copt)
    : ISimplePlayer(app_directory_absolute_path, options, opt, copt), stream_url_(stream_url) {}

std::string SimplePlayer::GetCurrentUrlName() const {
  return stream_url_.Url();
}

void SimplePlayer::HandlePreExecEvent(core::events::PreExecEvent* event) {
  ISimplePlayer::HandlePreExecEvent(event);
  core::VideoState* stream = CreateStream("0", stream_url_, GetStreamOptions());
  SetStream(stream);
}

void SimplePlayer::HandlePostExecEvent(core::events::PostExecEvent* event) {
  ISimplePlayer::HandlePostExecEvent(event);
}

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
