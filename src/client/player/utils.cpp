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

#include "client/player/utils.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <common/utils.h>

namespace fasto {
namespace fastotv {
namespace client {

bool DownloadFileToBuffer(const common::uri::Uri& uri, common::buffer_t* buff) {
  if (!uri.IsValid() || !buff) {
    return false;
  }

  AVFormatContext* ic = avformat_alloc_context();
  if (!ic) {
    return false;
  }

  std::string uri_str;
  if (uri.GetScheme() == common::uri::Uri::file) {
    common::uri::Upath upath = uri.GetPath();
    uri_str = upath.GetPath();
  } else {
    uri_str = uri.GetUrl();
  }

  const char* in_filename = common::utils::c_strornull(uri_str);
  int open_result = avformat_open_input(&ic, in_filename, NULL, NULL);
  if (open_result < 0) {
    avformat_free_context(ic);
    ic = NULL;
    return false;
  }
  AVPacket pkt;
  int ret = av_read_frame(ic, &pkt);
  if (ret < 0) {
    avformat_free_context(ic);
    ic = NULL;
    return false;
  }

  *buff = common::buffer_t(pkt.data, pkt.data + pkt.size);
  av_packet_unref(&pkt);
  avformat_free_context(ic);
  ic = NULL;
  return true;
}

}  // namespace client
}  // namespace fastotv
}  // namespace fasto
