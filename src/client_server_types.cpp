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

#include "client_server_types.h"

#include <common/convert2string.h>

namespace fasto {
namespace fastotv {

std::string Encode(const std::string& data) {
  std::string enc_data = common::HexEncode(data, false);
  return enc_data;
}

std::string Decode(const std::string& data) {
  common::buffer_t dec = common::HexDecode(data);
  return common::ConvertToString(dec);
}
}
}
