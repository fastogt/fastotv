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

#include "channel_info.h"

#include <string>

#include "third-party/json-c/json-c/json.h"  // for json_object_...

#include <common/sprintf.h>
#include <common/convert2string.h>

namespace fasto {
namespace fastotv {

json_object* MakeJobjectFromChannels(const channels_t& channels) {
  json_object* jchannels = json_object_new_array();
  for (Url url : channels) {
    json_object_array_add(jchannels, Url::MakeJobject(url));
  }
  return jchannels;
}

channels_t MakeChannelsClass(json_object* obj) {
  channels_t chan;
  int len = json_object_array_length(obj);
  for (int i = 0; i < len; ++i) {
    chan.push_back(Url::MakeClass(json_object_array_get_idx(obj, i)));
  }
  return chan;
}

}  // namespace fastotv
}  // namespace fasto
