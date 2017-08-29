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

#include <string>  // for string

#include <common/bounded_value.h>
#include <common/time.h>
#include <common/types.h>  // for time64_t

namespace fastotv {

typedef std::string stream_id;  // must be unique
static const stream_id invalid_stream_id = stream_id();

typedef std::string login_t;      // unique, user email now
typedef std::string device_id_t;  // unique, mongodb id, registered by user
typedef size_t bandwidth_t;       // bytes/s
typedef common::time64_t timestamp_t;

typedef std::string serializet_t;

typedef common::BoundedValue<int8_t, 0, 100> audio_volume_t;

enum ChannelType { UNKNOWN_CHANNEL, OFFICAL_CHANNEL, PRIVATE_CHANNEL };

}  // namespace fastotv
