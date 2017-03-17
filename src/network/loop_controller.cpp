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

#include "loop_controller.h"

#include <stdlib.h>

#include "tcp/tcp_server.h"

#include <common/threads/thread_manager.h>

namespace fasto {
namespace fastotv {
namespace network {

ILoopController::ILoopController() : loop_(nullptr), handler_(nullptr) {}

int ILoopController::Exec() {
  CHECK(!handler_);
  CHECK(!loop_);

  handler_ = CreateHandler();
  if (!handler_) {
    return EXIT_FAILURE;
  }

  loop_ = CreateServer(handler_);
  if (!loop_) {
    delete handler_;
    handler_ = nullptr;
    return EXIT_FAILURE;
  }

  return loop_->Exec();
}

void ILoopController::Start() {
  Started();
}

void ILoopController::Stop() {
  if (loop_) {
    loop_->Stop();
  }

  Stoped();
}

void ILoopController::ExecInLoopThread(async_loop_exec_function_t func) const {
  if (loop_) {
    loop_->ExecInLoopThread(func);
  }
}

ILoopController::~ILoopController() {
  delete loop_;
  delete handler_;
}

ILoopThreadController::ILoopThreadController() : ILoopController(), loop_thread_() {
  loop_thread_ = THREAD_MANAGER()->CreateThread(&ILoopController::Exec, this);
}

ILoopThreadController::~ILoopThreadController() {}

int ILoopThreadController::Join() {
  return loop_thread_->JoinAndGet();
}

void ILoopThreadController::Started() {
  bool result = loop_thread_->Start();
  DCHECK(result);
}

void ILoopThreadController::Stoped() {
  Join();
}

}
}  // namespace fastotv
}  // namespace fasto
