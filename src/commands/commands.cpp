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

#include "commands/commands.h"

namespace fastotv {

std::string CmdIdToString(cmd_id_t id) {
  static const std::string seq_names[] = {"REQUEST", "RESPONCE", "APPROVE"};
  if (id < SIZEOFMASS(seq_names)) {
    return seq_names[id];
  }

  DNOTREACHED();
  return std::string();
}

common::Error StableCommand(const std::string& command, std::string* stabled_command) {
  if (command.empty() || !stabled_command) {
    return common::make_error("Prepare commands, invalid input");
  }

  size_t pos = command.find_last_of(END_OF_COMMAND);
  if (pos == std::string::npos) {
    return common::make_error("UNKNOWN SEQUENCE: " + command);
  }

  *stabled_command = command.substr(0, pos - 1);
  return common::Error();
}

common::Error ParseCommand(const std::string& command, cmd_id_t* cmd_id, cmd_seq_t* seq_id, std::string* cmd_str) {
  if (command.empty() || !cmd_id || !seq_id || !cmd_str) {
    return common::make_error("Parse command, invalid input");
  }

  std::string stabled_command;
  common::Error err = StableCommand(command, &stabled_command);
  if (err) {
    return err;
  }

  char* star_seq = NULL;
  cmd_id_t lcmd_id = strtoul(stabled_command.c_str(), &star_seq, 10);
  if (*star_seq != ' ') {
    return common::make_error("PROBLEM EXTRACTING SEQUENCE: " + command);
  }

  const char* id_ptr = strchr(star_seq + 1, ' ');
  if (!id_ptr) {
    return common::make_error("PROBLEM EXTRACTING ID: " + command);
  }

  ptrdiff_t len_seq = id_ptr - (star_seq + 1);
  cmd_seq_t lseq_id = cmd_seq_t(star_seq + 1, len_seq);

  *cmd_id = lcmd_id;
  *seq_id = lseq_id;
  *cmd_str = id_ptr + 1;
  return common::Error();
}

}  // namespace fastotv
