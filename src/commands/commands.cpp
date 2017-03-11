/*  Copyright (C) 2014-2016 FastoGT. All right reserved.

    This file is part of SiteOnYourDevice.

    SiteOnYourDevice is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SiteOnYourDevice is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SiteOnYourDevice.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "commands/commands.h"

#include <common/utils.h>
#include <common/convert2string.h>

namespace fasto {
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
    return common::make_error_value("Invalid input", common::Value::E_ERROR);
  }

  size_t pos = command.find_last_of(END_OF_COMMAND);
  if (pos == std::string::npos) {
    return common::make_error_value("UNKNOWN SEQUENCE: " + command, common::Value::E_ERROR);
  }

  *stabled_command = command.substr(0, pos - 1);
  return common::Error();
}

common::Error ParseCommand(const std::string& command,
                           cmd_id_t* cmd_id,
                           cmd_seq_t* seq_id,
                           std::string* cmd_str) {
  if (command.empty() || !cmd_id || !seq_id || !cmd_str) {
    return common::make_error_value("Invalid input", common::Value::E_ERROR);
  }

  std::string stabled_command;
  common::Error err = StableCommand(command, &stabled_command);
  if (err && err->isError()) {
    return err;
  }

  char* star_seq = NULL;
  cmd_id_t lcmd_id = strtoul(stabled_command.c_str(), &star_seq, 10);
  if (*star_seq != ' ') {
    return common::make_error_value("PROBLEM EXTRACTING SEQUENCE: " + command,
                                    common::Value::E_ERROR);
  }

  const char* id_ptr = strchr(star_seq + 1, ' ');
  if (!id_ptr) {
    return common::make_error_value("PROBLEM EXTRACTING ID: " + command, common::Value::E_ERROR);
  }

  ptrdiff_t len_seq = id_ptr - (star_seq + 1);
  cmd_seq_t lseq_id = cmd_seq_t(star_seq + 1, len_seq);

  *cmd_id = lcmd_id;
  *seq_id = lseq_id;
  *cmd_str = id_ptr + 1;
  return common::Error();
}
}
}
