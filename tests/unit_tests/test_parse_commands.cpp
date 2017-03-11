#include <gtest/gtest.h>

#include "commands/commands.h"

using namespace fasto::fastotv;

TEST(commands, parse_commands) {
  char buff[MAX_COMMAND_SIZE] = {0};
  int res = common::SNPrintf(buff, MAX_COMMAND_SIZE, PING_COMMAND_RESP_SUCCESS, RESPONCE_COMMAND, 0);
  cmd_id_t id;
  common::Error err = ParseCommand(buff, &id);

  ASSERT_TRUE(!err);
}
