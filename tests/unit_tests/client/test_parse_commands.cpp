#include <gtest/gtest.h>

#include "client/commands.h"

using namespace fasto::fastotv;

TEST(commands, parse_commands) {
  const cmd_seq_t seq_id_const = "10";
  cmd_request_t req = client::PingRequest(seq_id_const);
  cmd_id_t cmd_id;
  cmd_seq_t seq_id;
  std::string command_str;
  common::Error err = ParseCommand(req.cmd(), &cmd_id, &seq_id, &command_str);

  ASSERT_TRUE(!err);
  ASSERT_EQ(cmd_id, REQUEST_COMMAND);
  ASSERT_EQ(seq_id, seq_id_const);
  ASSERT_EQ(command_str, CLIENT_PING_COMMAND " ");
}
