#include <gtest/gtest.h>

#include "server/commands.h"

using namespace fastotv;

void TestParseRequestComand(cmd_request_t req, const std::string& etalon_cmd) {
  cmd_seq_t id_seq = req.GetId();
  cmd_id_t cmd_id;
  cmd_seq_t seq_id;
  std::string command_str;
  common::Error err = ParseCommand(req.GetCmd(), &cmd_id, &seq_id, &command_str);
  ASSERT_TRUE(!err);
  ASSERT_EQ(cmd_id, REQUEST_COMMAND);
  ASSERT_EQ(seq_id, id_seq);
  ASSERT_EQ(command_str, etalon_cmd);
}

TEST(commands, parse_commands) {
  const cmd_seq_t who_are_you_id_seq = "1234";
  cmd_request_t who_are_you_req = server::WhoAreYouRequest(who_are_you_id_seq);
  TestParseRequestComand(who_are_you_req, SERVER_WHO_ARE_YOU);

  const cmd_seq_t info_id_seq = "110";
  cmd_request_t sys_req = server::SystemInfoRequest(info_id_seq);
  TestParseRequestComand(sys_req, SERVER_GET_CLIENT_INFO);

  const cmd_seq_t ping_id_seq = "10";
  cmd_request_t ping_req = server::PingRequest(ping_id_seq);
  TestParseRequestComand(ping_req, SERVER_PING);
}
