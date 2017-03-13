#include <gtest/gtest.h>

#include "client/commands.h"

using namespace fasto::fastotv;

TEST(commands, parse_commands) {
  char buff[MAX_COMMAND_SIZE] = {0};
  const cmd_seq_t seq_id_const = "10";
  int res = common::SNPrintf(buff, MAX_COMMAND_SIZE, CLIENT_PING_COMMAND_COMMAND_RESP_SUCCSESS, RESPONCE_COMMAND, seq_id_const);
  cmd_id_t cmd_id;
  cmd_seq_t seq_id;
  std::string command_str;
  common::Error err = ParseCommand(buff, &cmd_id, &seq_id, &command_str);

  ASSERT_TRUE(!err);
  ASSERT_EQ(cmd_id, RESPONCE_COMMAND);
  ASSERT_EQ(seq_id, seq_id_const);
  ASSERT_EQ(command_str, CLIENT_PING_COMMAND);
}
