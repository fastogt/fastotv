#include <gtest/gtest.h>

#include "server/commands.h"

using namespace fastotv;

void TestParseRequestComand(common::protocols::three_way_handshake::cmd_request_t req, const std::string& etalon_cmd) {
  common::protocols::three_way_handshake::cmd_seq_t id_seq = req.GetId();
  common::protocols::three_way_handshake::cmd_id_t cmd_id;
  common::protocols::three_way_handshake::cmd_seq_t seq_id;
  std::string command_str;
  common::Error err = common::protocols::three_way_handshake::ParseCommand(req.GetCmd(), &cmd_id, &seq_id, &command_str);
  ASSERT_TRUE(!err);
  ASSERT_EQ(cmd_id, REQUEST_COMMAND);
  ASSERT_EQ(seq_id, id_seq);
  ASSERT_EQ(command_str, etalon_cmd);
}

TEST(commands, parse_commands) {
  const common::protocols::three_way_handshake::cmd_seq_t who_are_you_id_seq = "1234";
  common::protocols::three_way_handshake::cmd_request_t who_are_you_req = server::WhoAreYouRequest(who_are_you_id_seq);
  TestParseRequestComand(who_are_you_req, SERVER_WHO_ARE_YOU);

  const common::protocols::three_way_handshake::cmd_seq_t info_id_seq = "110";
  common::protocols::three_way_handshake::cmd_request_t sys_req = server::SystemInfoRequest(info_id_seq);
  TestParseRequestComand(sys_req, SERVER_GET_CLIENT_INFO);

  const common::protocols::three_way_handshake::cmd_seq_t ping_id_seq = "10";
  common::protocols::three_way_handshake::cmd_request_t ping_req = server::PingRequest(ping_id_seq);
  TestParseRequestComand(ping_req, SERVER_PING);
}
