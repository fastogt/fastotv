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

  *stabled_command = command.substr(pos);
  return common::Error();
}

common::Error ParseCommand(const std::string& command, cmd_id_t* seq) {
  if (command.empty() || !seq) {
    return common::make_error_value("Invalid input", common::Value::E_ERROR);
  }

  std::string stabled_command;
  common::Error err = StableCommand(command, &stabled_command);
  if (err && err->isError()) {
    return err;
  }

  std::vector<std::string> parts;
  size_t argc = common::Tokenize(stabled_command, " ", &parts);
  if (!argc) {
    return common::make_error_value("Invaid command line.", common::ErrorValue::E_ERROR);
  }

  cmd_id_t lseq = common::ConvertFromString<cmd_id_t>(parts[0]);

  *seq = lseq;
  return common::Error();
}

}
}
