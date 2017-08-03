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

#define HEX_ALGO 0
#define SNAPPY_ALGO 1

#define COMPRESS_ALGO SNAPPY_ALGO

#if COMPRESS_ALGO == HEX_ALGO
#include <common/compress/hex.h>
#else
#include <common/compress/snappy_compress.h>
#endif

namespace fasto {
namespace fastotv {

std::string Encode(const std::string& data) {
  std::string enc_data;
#if COMPRESS_ALGO == HEX_ALGO
  common::Error err = common::compress::EncodeHex(data, false, &enc_data);
  if (err && err->IsError()) {
    NOTREACHED();
    return std::string();
  }
#else
  common::Error err = common::compress::EncodeSnappy(data, &enc_data);
  if (err && err->IsError()) {
    NOTREACHED();
    return std::string();
  }
#endif

  CHECK(data == Decode(enc_data));
  return enc_data;
}

std::string Decode(const std::string& data) {
  std::string dec_data;
#if COMPRESS_ALGO == HEX_ALGO
  common::Error err = common::compress::DecodeHex(data, &dec_data);
  if (err && err->IsError()) {
    NOTREACHED();
    return std::string();
  }
#else
  common::Error err = common::compress::DecodeSnappy(data, &dec_data);
  if (err && err->IsError()) {
    NOTREACHED();
    return std::string();
  }
#endif

  //CHECK(data == Encode(dec_data));
  return dec_data;
}

}  // namespace fastotv
}  // namespace fasto
