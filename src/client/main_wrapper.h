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

#include <common/url.h>

// runtime_directory_absolute_path can be not equal pwd (used for pid file location)
int main_simple_player_application(int argc,
                                   char** argv,
                                   const common::uri::Uri& stream_url,
                                   const std::string& app_directory_absolute_path,
                                   const std::string& runtime_directory_absolute_path);

// runtime_directory_absolute_path can be not equal pwd (used for pid file location)
int main_single_application(int argc,
                            char** argv,
                            const std::string& app_directory_absolute_path,
                            const std::string& runtime_directory_absolute_path);
