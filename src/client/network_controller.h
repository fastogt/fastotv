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

#pragma once

#include <common/threads/types.h>

#include <string>

#include "infos.h"

#include "network/loop_controller.h"
#include "tv_config.h"

namespace fasto {
namespace fastotv {
namespace client {

class NetworkController : private fasto::fastotv::network::ILoopThreadController {
 public:
  NetworkController(const std::string& config_path);
  ~NetworkController();

  void Start();
  void Stop();
  AuthInfo authInfo() const;
  TvConfig config() const;
  void SetConfig(const TvConfig& config);

  void RequestChannels() const;

 private:
  void readConfig();
  void saveConfig();

  virtual network::tcp::ITcpLoopObserver* CreateHandler() override;
  virtual network::tcp::ITcpLoop* CreateServer(network::tcp::ITcpLoopObserver* handler) override;

  const std::string config_path_;
  TvConfig config_;
};

}  // namespace network
}  // namespace fastotv
}  // namespace fasto
