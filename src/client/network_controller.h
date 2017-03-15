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

#pragma once

#include <common/threads/types.h>

#include <string>

#include "infos.h"

#include "network/loop_controller.h"
#include "tv_config.h"

namespace fasto {
namespace fastotv {
namespace network {

class NetworkController : private ILoopThreadController {
 public:
  NetworkController(const std::string& config_path);
  ~NetworkController();

  void Start();
  void Stop();
  AuthInfo authInfo() const;
  TvConfig config() const;
  void setConfig(const TvConfig& config);

  void RequestChannels() const;

 private:
  void readConfig();
  void saveConfig();

  virtual tcp::ITcpLoopObserver* CreateHandler() override;
  virtual tcp::ITcpLoop* CreateServer(tcp::ITcpLoopObserver* handler) override;

  std::string config_path_;
  TvConfig config_;
};

}  // namespace network
}  // namespace fastotv
}  // namespace fasto
