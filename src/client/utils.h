/*  Copyright (C) 2014-2022 FastoGT. All right reserved.

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

#include <functional>

#include <common/types.h>
#include <common/uri/gurl.h>

namespace fastotv {
namespace client {

typedef std::function<bool()> quit_callback_t;
bool DownloadFileToBuffer(const common::uri::GURL& uri, common::char_buffer_t* buff, quit_callback_t cb);

}  // namespace client
}  // namespace fastotv
