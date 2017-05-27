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

#include "client/core/events/network_events.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {
namespace events {
BandwidtInfo::BandwidtInfo() : host(), bandwidth(0), host_type(UNKNOWN_SERVER) {}

BandwidtInfo::BandwidtInfo(const common::net::HostAndPort& host,
                           bandwidth_t band,
                           BandwidthHostType hs)
    : host(host), bandwidth(band), host_type(hs) {}

ConnectInfo::ConnectInfo() {}

ConnectInfo::ConnectInfo(const common::net::HostAndPort& host) : host(host) {}
}
}
}
}
}
