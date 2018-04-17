#include <gtest/gtest.h>

#include "server/responce_info.h"
#include "server/user_info.h"
#include "server/user_state_info.h"

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

  fastotv::server::UserInfo uinf(login, password, channel_info, fastotv::server::UserInfo::devices_t());
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

TEST(UserStateInfo, serialize_deserialize) {
  const fastotv::server::user_id_t user_id = "123fe";
  const bool connected = false;

  fastotv::server::UserStateInfo ust(user_id, "", connected);
  ASSERT_EQ(ust.GetUserId(), user_id);
  ASSERT_EQ(ust.IsConnected(), connected);

  serialize_t ser;
  common::Error err = ust.Serialize(&ser);
  ASSERT_TRUE(!err);
  fastotv::server::UserStateInfo dust;
  err = dust.DeSerialize(ser);
  ASSERT_TRUE(!err);

  ASSERT_EQ(ust.GetUserId(), dust.GetUserId());
  ASSERT_EQ(ust.GetDeviceId(), dust.GetDeviceId());
  ASSERT_EQ(ust, dust);
}

TEST(ResponceInfo, serialize_deserialize) {
  const std::string request_id = "req";
  const std::string state = "state";
  const std::string command = "comma";
  const std::string responce_json = "{}";

  fastotv::server::ResponceInfo ust(request_id, state, command, responce_json);
  ASSERT_EQ(ust.GetRequestId(), request_id);
  ASSERT_EQ(ust.GetState(), state);
  ASSERT_EQ(ust.GetCommand(), command);
  ASSERT_EQ(ust.GetResponceJson(), responce_json);

  serialize_t ser;
  common::Error err = ust.Serialize(&ser);
  ASSERT_TRUE(!err);
  fastotv::server::ResponceInfo dust;
  err = dust.DeSerialize(ser);
  ASSERT_TRUE(!err);

  ASSERT_EQ(ust, dust);
}
