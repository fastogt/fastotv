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

#include <string>

#include <common/smart_ptr.h>

#include <common/libev/loop_controller.h>

namespace common {
namespace threads {
template <typename RT>
class Thread;
}
}

namespace fasto {
namespace fastotv {
namespace client {

class IoService : public common::libev::ILoopController {
 public:
  IoService();
  virtual ~IoService();

  void Start();
  void Stop();

  void ConnectToServer() const;
  void DisconnectFromServer() const;
  void RequestServerInfo() const;
  void RequestChannels() const;

 private:
  using ILoopController::Exec;

  virtual common::libev::IoLoopObserver* CreateHandler() override;
  virtual common::libev::IoLoop* CreateServer(common::libev::IoLoopObserver* handler) override;

  virtual void HandleStarted() override;
  virtual void HandleStoped() override;

  common::shared_ptr<common::threads::Thread<int> > loop_thread_;
};

}  // namespace network
}  // namespace fastotv
}  // namespace fasto
