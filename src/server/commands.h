/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of SiteOnYourDevice.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "commands/commands.h"

// ping
#define SERVER_PING_COMMAND_COMMAND_RESP_FAIL_1S GENEATATE_FAIL_FMT(CLIENT_PING_COMMAND, "%s")
#define SERVER_PING_COMMAND_RESP_SUCCSESS GENEATATE_SUCCESS_FMT(CLIENT_PING_COMMAND, "pong")

// get_channels
#define SERVER_GET_CHANNELS_COMMAND_RESP_FAIL_1S GENEATATE_FAIL_FMT(CLIENT_GET_CHANNELS, "%s")
#define SERVER_GET_CHANNELS_COMMAND_RESP_SUCCSESS_1S \
  GENEATATE_SUCCESS_FMT(CLIENT_GET_CHANNELS, "%s")
