/*  Copyright (C) 2014-2019 FastoGT. All right reserved.

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

#include <gtest/gtest.h>

#include "server/user_info.h"

typedef fastotv::ChannelInfo::serialize_type serialize_t;

TEST(UserInfo, serialize_deserialize) {
  const std::string login = "palecc";
  const std::string password = "faf";

  const std::string name = "alex";
  const fastotv::stream_id stream_id = "123";
  const common::uri::Url url("http://localhost:8080/hls/69_avformat_test_alex_2/play.m3u8");
  const bool enable_video = false;
  const bool enable_audio = true;

  fastotv::EpgInfo epg_info(stream_id, url, name);
  fastotv::ChannelsInfo channel_info;
  channel_info.AddChannel(fastotv::ChannelInfo(epg_info, enable_audio, enable_video));

  fastotv::server::UserInfo uinf("11", login, password, channel_info, fastotv::server::UserInfo::devices_t(),
                                 fastotv::server::ACTIVE);
  ASSERT_EQ(uinf.GetLogin(), login);
  ASSERT_EQ(uinf.GetPassword(), password);
  ASSERT_EQ(uinf.GetChannelInfo(), channel_info);

  serialize_t ser;
  common::Error err = uinf.Serialize(&ser);
  ASSERT_TRUE(!err);
  fastotv::server::UserInfo duinf;
  err = duinf.DeSerialize(ser);
  ASSERT_TRUE(!err);

  ASSERT_EQ(uinf, duinf);

  const std::string json_channel =
      R"(
      {
        "login":"atopilski@gmail.com",
        "password":"1234",
        "channels":
        [
        {
          "epg":{
          "id":"59106ed9457cd9f4c3c0b78f",
          "url":"http://example.com:6969/127.ts",
          "display_name":"Alex TV",
          "icon":"/images/unknown_channel.png",
          "programs":[]},
          "video":true,
          "audio":true
        },
        {
          "epg":
          {
            "id":"592fa5778b385c798bd499fa",
            "url":"fiel://C:/msys64/home/Sasha/work/fastotv/tests/big_buck_bunny_1080p_h264.mov",
            "display_name":"Local",
            "icon":"/images/unknown_channel.png",
           "programs":[]
          },
          "video":true,
          "audio":true
        },
        {
          "epg":
          {
            "id":"592feb388b385c798bd499fb",
            "url":"file:///home/sasha/work/fastotv/tests/big_buck_bunny_1080p_h264.mov",
            "display_name":"Local2",
            "icon":"/images/unknown_channel.png",
            "programs":[]
          },
          "video":true,
          "audio":true
        }
        ]
      }
      )";

  err = uinf.SerializeFromString(json_channel, &ser);
  ASSERT_TRUE(!err);

  err = duinf.DeSerialize(ser);
  ASSERT_TRUE(!err);
  fastotv::ChannelsInfo ch = duinf.GetChannelInfo();
  ASSERT_EQ(duinf.GetLogin(), "atopilski@gmail.com");
  ASSERT_EQ(duinf.GetPassword(), "1234");
  ASSERT_EQ(ch.GetSize(), 3);
}
