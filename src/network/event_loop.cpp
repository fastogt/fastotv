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

#include "network/event_loop.h"

#include <stdlib.h>

#define ev_async_cb_init_fasto(ev, cb) ev_async_init(&(ev->async), cb)
#define ev_async_cb_start_fasto(loop, ev) ev_async_start(loop, &(ev->async))
#define ev_async_cb_stop_fasto(loop, ev) ev_async_stop(loop, &(ev->async))
#define ev_async_cb_send_fasto(loop, ev) ev_async_send(loop, &(ev->async))

namespace {

struct fasto_async_cb {
  ev_async async;
  fasto::fastotv::async_loop_exec_function_t func;
};

void async_exec_cb(struct ev_loop* loop, struct ev_async* watcher, int revents) {
  ev_async_stop(loop, watcher);
  fasto_async_cb* ioclient = reinterpret_cast<fasto_async_cb*>(watcher);
  ioclient->func();
  free(ioclient);
}

}  // namespace

namespace fasto {
namespace fastotv {

EvLoopObserver::~EvLoopObserver() {}

LibEvLoop::LibEvLoop()
    : loop_(ev_loop_new(0)),
      observer_(nullptr),
      exec_id_(),
      async_stop_((struct ev_async*)calloc(1, sizeof(struct ev_async))) {
  ev_async_init(async_stop_, stop_cb);
  ev_set_userdata(loop_, this);
}

LibEvLoop::~LibEvLoop() {
  ev_loop_destroy(loop_);

  free(async_stop_);
  async_stop_ = NULL;
}

void LibEvLoop::setObserver(EvLoopObserver* observer) {
  observer_ = observer;
}

void LibEvLoop::start_io(ev_io* io) {
  CHECK(isLoopThread());
  ev_io_start(loop_, io);
}

void LibEvLoop::stop_io(ev_io* io) {
  CHECK(isLoopThread());
  ev_io_stop(loop_, io);
}

void LibEvLoop::start_timer(ev_timer* timer) {
  CHECK(isLoopThread());
  ev_timer_start(loop_, timer);
}

void LibEvLoop::stop_timer(ev_timer* timer) {
  CHECK(isLoopThread());
  ev_timer_stop(loop_, timer);
}

void LibEvLoop::execInLoopThread(async_loop_exec_function_t async_cb) {
  if (isLoopThread()) {
    async_cb();
  } else {
    fasto_async_cb* cb = (struct fasto_async_cb*)calloc(1, sizeof(struct fasto_async_cb));
    cb->func = async_cb;

    ev_async_cb_init_fasto(cb, async_exec_cb);
    ev_async_cb_start_fasto(loop_, cb);
    ev_async_cb_send_fasto(loop_, cb);
  }
}

int LibEvLoop::exec() {
  exec_id_ = common::threads::PlatformThread::currentId();

  ev_async_start(loop_, async_stop_);
  if (observer_) {
    observer_->preLooped(this);
  }
  ev_loop(loop_, 0);
  if (observer_) {
    observer_->postLooped(this);
  }
  return EXIT_SUCCESS;
}

bool LibEvLoop::isLoopThread() const {
  return exec_id_ == common::threads::PlatformThread::currentId();
}

void LibEvLoop::stop() {
  ev_async_send(loop_, async_stop_);
}

void LibEvLoop::stop_cb(struct ev_loop* loop, struct ev_async* watcher, int revents) {
  LibEvLoop* evloop = reinterpret_cast<LibEvLoop*>(ev_userdata(loop));
  ev_async_stop(loop, evloop->async_stop_);
  if (evloop->observer_) {
    evloop->observer_->stoped(evloop);
  }
  ev_unloop(loop, EVUNLOOP_ONE);
}

}  // namespace fastotv
}  // namespace fasto
