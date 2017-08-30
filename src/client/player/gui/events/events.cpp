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

#include "client/player/gui/events/events.h"

#include <common/time.h>

namespace fastotv {
namespace client {
namespace player {
namespace core {
namespace events {

TimeInfo::TimeInfo() : time_millisecond(common::time::current_mstime()) {}

PreExecInfo::PreExecInfo(int code) : code(code) {}

PostExecInfo::PostExecInfo(int code) : code(code) {}

}  // namespace events
}  // namespace core
}  // namespace player
}  // namespace client
}  // namespace fastotv
