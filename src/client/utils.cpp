/*  Copyright (C) 2014-2020 FastoGT. All right reserved.

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

#include "client/utils.h"

#include <string>

extern "C" {
#include <libavformat/avformat.h>
}

#include <player/media/types.h>

namespace fastotv {
namespace client {
namespace {
class CallbackHolder {
 public:
  explicit CallbackHolder(quit_callback_t cb) : is_quit(cb) {}
  static int download_interrupt_callback(void* user_data) {
    CallbackHolder* holder = static_cast<CallbackHolder*>(user_data);
    if (holder->is_quit()) {
      return 1;
    }

    return 0;
  }

 private:
  const quit_callback_t is_quit;
};
}  // namespace

bool DownloadFileToBuffer(const common::uri::GURL& uri, common::char_buffer_t* buff, quit_callback_t cb) {
  if (!uri.is_valid() || !buff || !cb) {
    return false;
  }

  const std::string url_str = fastoplayer::media::make_url(uri);
  if (url_str.empty()) {
    return false;
  }

  AVFormatContext* ic = avformat_alloc_context();
  if (!ic) {
    return false;
  }

  const char* in_filename = url_str.c_str();
  CallbackHolder holder(cb);
  ic->interrupt_callback.callback = CallbackHolder::download_interrupt_callback;
  ic->interrupt_callback.opaque = &holder;
  int open_result = avformat_open_input(&ic, in_filename, nullptr, nullptr);
  if (open_result < 0) {
    avformat_free_context(ic);
    ic = nullptr;
    return false;
  }
  AVPacket pkt;
  int ret = av_read_frame(ic, &pkt);
  if (ret < 0) {
    avformat_free_context(ic);
    ic = nullptr;
    return false;
  }

  *buff = common::char_buffer_t(pkt.data, pkt.data + pkt.size);
  av_packet_unref(&pkt);
  avformat_free_context(ic);
  ic = nullptr;
  return true;
}

}  // namespace client
}  // namespace fastotv
