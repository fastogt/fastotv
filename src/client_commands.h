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

// who are you
#define CLIENT_WHO_ARE_YOU_COMMAND_RESP_FAIL_1S \
  GENEATATE_FAIL_FMT(SERVER_WHO_ARE_YOU_COMMAND, "%s")
#define CLIENT_WHO_ARE_YOU_COMMAND_RESP_SUCCSESS_1S \
  GENEATATE_SUCCESS_FMT(SERVER_WHO_ARE_YOU_COMMAND, "%s")

// http
#define CLIENT_PLEASE_CONNECT_HTTP_COMMAND_RESP_FAIL_1S \
  GENEATATE_FAIL_FMT(SERVER_PLEASE_CONNECT_HTTP_COMMAND, "%s")
#define CLIENT_PLEASE_CONNECT_HTTP_COMMAND_RESP_SUCCSESS_1S \
  GENEATATE_SUCCESS_FMT(SERVER_PLEASE_CONNECT_HTTP_COMMAND, "%s")

// websocket
#define CLIENT_PLEASE_CONNECT_WEBSOCKET_COMMAND_RESP_FAIL_1S \
  GENEATATE_FAIL_FMT(SERVER_PLEASE_CONNECT_WEBSOCKET_COMMAND, "%s")
#define CLIENT_PLEASE_CONNECT_WEBSOCKET_COMMAND_RESP_SUCCSESS_1S \
  GENEATATE_SUCCESS_FMT(SERVER_PLEASE_CONNECT_WEBSOCKET_COMMAND, "%s")

// system info
#define CLIENT_PLEASE_SYSTEM_INFO_COMMAND_RESP_FAIL_1S \
  GENEATATE_FAIL_FMT(SERVER_PLEASE_SYSTEM_INFO_COMMAND, "%s")
#define CLIENT_PLEASE_SYSTEM_INFO_COMMAND_RESP_SUCCSESS_1J \
  GENEATATE_SUCCESS_FMT(SERVER_PLEASE_SYSTEM_INFO_COMMAND, "'%s'")

// config
#define CLIENT_PLEASE_CONFIG_COMMAND_RESP_FAIL_1S \
  GENEATATE_FAIL_FMT(SERVER_PLEASE_CONFIG_COMMAND, "%s")
#define CLIENT_PLEASE_CONFIG_COMMAND_RESP_SUCCSESS_1J \
  GENEATATE_SUCCESS_FMT(SERVER_PLEASE_CONFIG_COMMAND, "'%s'")

// set_config
#define CLIENT_PLEASE_SET_CONFIG_COMMAND_RESP_FAIL_1S \
  GENEATATE_FAIL_FMT(SERVER_PLEASE_SET_CONFIG_COMMAND, "%s")
#define CLIENT_PLEASE_SET_CONFIG_COMMAND_RESP_SUCCSESS \
  GENEATATE_SUCCESS_FMT(SERVER_PLEASE_SET_CONFIG_COMMAND, "")
