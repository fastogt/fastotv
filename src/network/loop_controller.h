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

#include <common/smart_ptr.h>
#include <common/threads/thread.h>

#include "network/types.h"

namespace fasto {
namespace fastotv {
namespace network {

namespace tcp {
class ITcpLoop;
class ITcpLoopObserver;
}

class ILoopController {
 public:
  ILoopController();
  virtual ~ILoopController();

  void Start();
  int Exec();
  void Stop();
  void ExecInLoopThread(async_loop_exec_function_t func) const;

 protected:
  tcp::ITcpLoop* loop_;
  tcp::ITcpLoopObserver* handler_;

 private:
  virtual tcp::ITcpLoopObserver* CreateHandler() = 0;
  virtual tcp::ITcpLoop* CreateServer(tcp::ITcpLoopObserver* handler) = 0;
  virtual void Started() = 0;
  virtual void Stoped() = 0;
};

class ILoopThreadController : public ILoopController {
 public:
  ILoopThreadController();
  virtual ~ILoopThreadController();

  int Join();

 private:
  using ILoopController::Exec;

  virtual tcp::ITcpLoopObserver* CreateHandler() override = 0;
  virtual tcp::ITcpLoop* CreateServer(tcp::ITcpLoopObserver* handler) override = 0;

  virtual void Started() override;
  virtual void Stoped() override;

  common::shared_ptr<common::threads::Thread<int> > loop_thread_;
};
}
}  // namespace fastotv
}  // namespace fasto
