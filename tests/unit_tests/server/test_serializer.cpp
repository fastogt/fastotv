#include <gtest/gtest.h>

#include "server/user_info.h"
#include "server/user_state_info.h"
#include "server/responce_info.h"

typedef fasto::fastotv::Url::serialize_type serialize_t;

TEST(UserInfo, serialize_deserialize) {
  const std::string login = "palecc";
  const std::string password = "faf";
  fasto::fastotv::AuthInfo auth_info(login, password);

  const std::string name = "alex";
  const fasto::fastotv::stream_id stream_id = "123";
  const common::uri::Uri url("http://localhost:8080/hls/69_avformat_test_alex_2/play.m3u8");
  const bool enable_video = false;
  const bool enable_audio = true;

  fasto::fastotv::ChannelsInfo channel_info;
  channel_info.AddChannel(fasto::fastotv::Url(stream_id, url, name, enable_audio, enable_video));

  fasto::fastotv::server::UserInfo uinf(auth_info, channel_info);
  ASSERT_EQ(uinf.GetAuthInfo(), auth_info);
  ASSERT_EQ(uinf.GetChannelInfo(), channel_info);

  serialize_t ser;
  common::Error err = uinf.Serialize(&ser);
  ASSERT_TRUE(!err);
  fasto::fastotv::server::UserInfo duinf;
  err = uinf.DeSerialize(ser, &duinf);
  ASSERT_TRUE(!err);

  ASSERT_EQ(uinf, duinf);
}

TEST(UserStateInfo, serialize_deserialize) {
  const fasto::fastotv::server::user_id_t user_id = "123fe";
  const bool connected = false;

  fasto::fastotv::server::UserStateInfo ust(user_id, connected);
  ASSERT_EQ(ust.GetUserId(), user_id);
  ASSERT_EQ(ust.IsConnected(), connected);

  serialize_t ser;
  common::Error err = ust.Serialize(&ser);
  ASSERT_TRUE(!err);
  fasto::fastotv::server::UserStateInfo dust;
  err = ust.DeSerialize(ser, &dust);
  ASSERT_TRUE(!err);

  ASSERT_EQ(ust, dust);
}

TEST(ResponceInfo, serialize_deserialize) {
  const std::string request_id = "req";
  const std::string state = "state";
  const std::string command = "comma";
  const std::string responce_json = "{}";

  fasto::fastotv::server::ResponceInfo ust(request_id, state, command, responce_json);
  ASSERT_EQ(ust.GetRequestId(), request_id);
  ASSERT_EQ(ust.GetState(), state);
  ASSERT_EQ(ust.GetCommand(), command);
  ASSERT_EQ(ust.GetResponceJson(), responce_json);

  serialize_t ser;
  common::Error err = ust.Serialize(&ser);
  ASSERT_TRUE(!err);
  fasto::fastotv::server::ResponceInfo dust;
  err = ust.DeSerialize(ser, &dust);
  ASSERT_TRUE(!err);

  ASSERT_EQ(ust, dust);
}
