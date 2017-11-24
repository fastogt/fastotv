#include <gtest/gtest.h>

#include "client/commands.h"

using namespace fastotv;

TEST(commands, parse_commands) {
  const common::protocols::three_way_handshake::cmd_seq_t seq_id_const = "10";
  common::protocols::three_way_handshake::cmd_request_t req = client::PingRequest(seq_id_const);
  common::protocols::three_way_handshake::cmd_id_t cmd_id;
  common::protocols::three_way_handshake::cmd_seq_t seq_id;
  std::string command_str;
  common::Error err = common::protocols::three_way_handshake::ParseCommand(req.GetCmd(), &cmd_id, &seq_id, &command_str);

  ASSERT_TRUE(!err);
  ASSERT_EQ(cmd_id, REQUEST_COMMAND);
  ASSERT_EQ(seq_id, seq_id_const);
  ASSERT_EQ(command_str, CLIENT_PING);
}
