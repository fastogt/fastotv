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

#include <ev.h>

#include <common/threads/platform_thread.h>

#include "network/types.h"

namespace fasto {
namespace fastotv {
namespace network {


class EvLoopObserver {
 public:
  virtual ~EvLoopObserver();

  virtual void PreLooped(class LibEvLoop* loop) = 0;
  virtual void Stoped(class LibEvLoop* loop) = 0;
  virtual void PostLooped(class LibEvLoop* loop) = 0;
};

class LibEvLoop {
 public:
  LibEvLoop();
  ~LibEvLoop();

  void setObserver(EvLoopObserver* observer);

  void StartIO(ev_io* io);
  void StopIO(ev_io* io);

  void StartTimer(ev_timer* timer);
  void StopTimer(ev_timer* timer);

  void ExecInLoopThread(async_loop_exec_function_t async_cb) const;

  int Exec();
  void Stop();

  bool IsLoopThread() const;

 private:
  static void stop_cb(struct ev_loop* loop, struct ev_async* watcher, int revents);

  struct ev_loop* const loop_;
  EvLoopObserver* observer_;
  common::threads::platform_thread_id_t exec_id_;
  ev_async* async_stop_;
};

}
}  // namespace fastotv
}  // namespace fasto
