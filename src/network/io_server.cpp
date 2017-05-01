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

#include "network/io_server.h"

#include <string>
#include <vector>

#include <common/logger.h>
#include <common/threads/types.h>

#include "network/tcp/tcp_client.h"

namespace {

typedef common::unique_lock<common::mutex> lock_t;
common::mutex g_exists_loops_mutex_;
std::vector<fasto::fastotv::network::IoLoop*> g_exists_loops_;

}  // namespace

namespace fasto {
namespace fastotv {
namespace network {

class LoopTimer : public common::patterns::id_counter<LoopTimer, timer_id_t> {
 public:
  explicit LoopTimer(IoLoop* server)
      : server_(server), timer_((struct ev_timer*)calloc(1, sizeof(struct ev_timer))) {
    timer_->data = this;
  }

  IoLoop* server() const { return server_; }

  ~LoopTimer() { free(timer_); }

  IoLoop* server_;
  struct ev_timer* const timer_;
};

IoLoop::IoLoop(IoLoopObserver* observer)
    : loop_(new LibEvLoop), observer_(observer), clients_(), timers_(), id_() {
  loop_->setObserver(this);
}

IoLoop::~IoLoop() {
  delete loop_;
}

int IoLoop::Exec() {
  int res = loop_->Exec();
  return res;
}

void IoLoop::Stop() {
  loop_->Stop();
}

void IoLoop::RegisterClient(const common::net::socket_info& info) {
  IoClient* client = CreateClient(info);
  RegisterClient(client);
}

void IoLoop::UnRegisterClient(IoClient* client) {
  CHECK(client->Server() == this);
  loop_->StopIO(client->read_write_io_);

  if (observer_) {
    observer_->Moved(client);
  }

  client->server_ = nullptr;
  clients_.erase(std::remove(clients_.begin(), clients_.end(), client), clients_.end());
  INFO_LOG() << "Successfully unregister client[" << client->FormatedName() << "], from server["
             << FormatedName() << "], " << clients_.size() << " client(s) connected.";
}

void IoLoop::RegisterClient(IoClient* client) {
  if (client->Server()) {
    CHECK(client->Server() == this);
  }

  // Initialize and start watcher to read client requests
  ev_io_init(client->read_write_io_, read_write_cb, client->Fd(), client->Flags());
  loop_->StartIO(client->read_write_io_);

  if (observer_) {
    observer_->Accepted(client);
  }

  client->server_ = this;
  clients_.push_back(client);
  INFO_LOG() << "Successfully connected with client[" << client->FormatedName() << "], from server["
             << FormatedName() << "], " << clients_.size() << " client(s) connected.";
}

void IoLoop::CloseClient(IoClient* client, common::Error err) {
  CHECK(client->Server() == this);
  loop_->StopIO(client->read_write_io_);

  if (observer_) {
    observer_->Closed(client, err);
  }
  clients_.erase(std::remove(clients_.begin(), clients_.end(), client), clients_.end());
  INFO_LOG() << "Successfully disconnected client[" << client->FormatedName() << "], from server["
             << FormatedName() << "], " << clients_.size() << " client(s) connected.";
}

timer_id_t IoLoop::CreateTimer(double sec, double repeat) {
  LoopTimer* timer = new LoopTimer(this);
  ev_timer_init(timer->timer_, timer_cb, sec, repeat);
  loop_->StartTimer(timer->timer_);
  timers_.push_back(timer);
  return timer->id();
}

void IoLoop::RemoveTimer(timer_id_t id) {
  for (std::vector<LoopTimer*>::iterator it = timers_.begin(); it != timers_.end(); ++it) {
    LoopTimer* timer = *it;
    if (timer->id() == id) {
      timers_.erase(it);
      delete timer;
      return;
    }
  }
}

common::patterns::id_counter<IoLoop>::type_t IoLoop::GetId() const {
  return id_.id();
}

void IoLoop::ExecInLoopThread(async_loop_exec_function_t func) {
  loop_->ExecInLoopThread(func);
}

bool IoLoop::IsLoopThread() const {
  return loop_->IsLoopThread();
}

void IoLoop::ChangeFlags(IoClient* client) {
  if (client->Flags() != client->read_write_io_->events) {
    loop_->StopIO(client->read_write_io_);
    ev_io_set(client->read_write_io_, client->Fd(), client->Flags());
    loop_->StartIO(client->read_write_io_);
  }
}

IoLoop* IoLoop::FindExistLoopByPredicate(std::function<bool(IoLoop*)> pred) {
  if (!pred) {
    return nullptr;
  }

  lock_t loc(g_exists_loops_mutex_);
  for (size_t i = 0; i < g_exists_loops_.size(); ++i) {
    IoLoop* loop = g_exists_loops_[i];
    if (loop && pred(loop)) {
      return loop;
    }
  }

  return nullptr;
}

std::vector<IoClient*> IoLoop::Clients() const {
  return clients_;
}

void IoLoop::SetName(const std::string& name) {
  name_ = name;
}

std::string IoLoop::Name() const {
  return name_;
}

std::string IoLoop::FormatedName() const {
  return common::MemSPrintf("[%s][%s(%llu)]", Name(), ClassName(), GetId());
}

void IoLoop::read_write_cb(struct ev_loop* loop, struct ev_io* watcher, int revents) {
  IoClient* pclient = reinterpret_cast<IoClient*>(watcher->data);
  IoLoop* pserver = pclient->Server();
  LibEvLoop* evloop = reinterpret_cast<LibEvLoop*>(ev_userdata(loop));
  CHECK(pserver && pserver->loop_ == evloop);

  if (EV_ERROR & revents) {
    DNOTREACHED();
    return;
  }

  if (revents & EV_READ) {
    if (pserver->observer_) {
      pserver->observer_->DataReceived(pclient);
    }
  }

  if (revents & EV_WRITE) {
    if (pserver->observer_) {
      pserver->observer_->DataReadyToWrite(pclient);
    }
  }
}

void IoLoop::timer_cb(struct ev_loop* loop, struct ev_timer* timer, int revents) {
  LoopTimer* ptimer = reinterpret_cast<LoopTimer*>(timer->data);
  IoLoop* pserver = ptimer->server();
  LibEvLoop* evloop = reinterpret_cast<LibEvLoop*>(ev_userdata(loop));
  CHECK(pserver && pserver->loop_ == evloop);

  if (EV_ERROR & revents) {
    DNOTREACHED();
    return;
  }

  DCHECK(revents & EV_TIMEOUT);
  if (pserver->observer_) {
    pserver->observer_->TimerEmited(pserver, ptimer->id());
  }
}

void IoLoop::PreLooped(LibEvLoop* loop) {
  UNUSED(loop);
  {
    lock_t loc(g_exists_loops_mutex_);
    g_exists_loops_.push_back(this);
  }

  if (observer_) {
    observer_->PreLooped(this);
  }
}

void IoLoop::Stoped(LibEvLoop* loop) {
  UNUSED(loop);
  const std::vector<LoopTimer*> timers = timers_;
  for (size_t i = 0; i < timers.size(); ++i) {
    LoopTimer* timer = timers[i];
    RemoveTimer(timer->id());
  }

  const std::vector<IoClient*> cl = Clients();

  for (size_t i = 0; i < cl.size(); ++i) {
    IoClient* client = cl[i];
    client->Close(common::make_error_value("Stoped", common::Value::E_ERROR));
    delete client;
  }
}

void IoLoop::PostLooped(LibEvLoop* loop) {
  UNUSED(loop);
  {
    lock_t loc(g_exists_loops_mutex_);
    g_exists_loops_.erase(std::remove(g_exists_loops_.begin(), g_exists_loops_.end(), this),
                          g_exists_loops_.end());
  }

  if (observer_) {
    observer_->PostLooped(this);
  }
}

IoLoopObserver::~IoLoopObserver() {
}
}
}  // namespace fastotv
}  // namespace fasto
