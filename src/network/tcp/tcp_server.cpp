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

#include <string>
#include <vector>

#include "network/tcp/tcp_server.h"

#include <common/logger.h>
#include <common/threads/types.h>

#include "network/tcp/tcp_client.h"

namespace {

#ifdef OS_WIN
struct WinsockInit {
  WinsockInit() {
    WSADATA d;
    int res = WSAStartup(MAKEWORD(2, 2), &d);
    if (res != 0) {
      DEBUG_MSG_PERROR("WSAStartup", res);
      exit(EXIT_FAILURE);
    }
  }
  ~WinsockInit() { WSACleanup(); }
} winsock_init;
#endif
struct SigIgnInit {
  SigIgnInit() {
#if defined(COMPILER_MINGW)
#elif defined(COMPILER_MSVC)
#else
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
      DEBUG_MSG_PERROR("signal", errno);
      exit(EXIT_FAILURE);
    }
#endif
  }
} sig_init;

typedef common::unique_lock<common::mutex> lock_t;
common::mutex g_exists_loops_mutex_;
std::vector<fasto::fastotv::tcp::ITcpLoop*> g_exists_loops_;

}  // namespace

namespace fasto {
namespace fastotv {
namespace tcp {

class LoopTimer : public common::patterns::id_counter<LoopTimer, timer_id_t> {
 public:
  explicit LoopTimer(ITcpLoop* server)
      : server_(server), timer_((struct ev_timer*)calloc(1, sizeof(struct ev_timer))) {
    timer_->data = this;
  }

  ITcpLoop* server() const { return server_; }

  ~LoopTimer() { free(timer_); }

  ITcpLoop* server_;
  struct ev_timer* const timer_;
};

ITcpLoop::ITcpLoop(ITcpLoopObserver* observer)
    : loop_(new LibEvLoop), observer_(observer), clients_(), timers_(), id_() {
  loop_->setObserver(this);
}

ITcpLoop::~ITcpLoop() {
  delete loop_;
}

int ITcpLoop::exec() {
  int res = loop_->exec();
  return res;
}

void ITcpLoop::stop() {
  loop_->stop();
}

void ITcpLoop::registerClient(const common::net::socket_info& info) {
  TcpClient* client = createClient(info);
  registerClient(client);
}

void ITcpLoop::unregisterClient(TcpClient* client) {
  CHECK(client->server() == this);
  loop_->stop_io(client->read_write_io_);

  if (observer_) {
    observer_->moved(client);
  }

  client->server_ = nullptr;
  clients_.erase(std::remove(clients_.begin(), clients_.end(), client), clients_.end());
  INFO_LOG() << "Successfully unregister client[" << client->formatedName() << "], from server["
             << formatedName() << "], " << clients_.size() << " client(s) connected.";
}

void ITcpLoop::registerClient(TcpClient* client) {
  if (client->server()) {
    CHECK(client->server() == this);
  }

  // Initialize and start watcher to read client requests
  ev_io_init(client->read_write_io_, read_write_cb, client->fd(), client->flags());
  loop_->start_io(client->read_write_io_);

  if (observer_) {
    observer_->accepted(client);
  }

  client->server_ = this;
  clients_.push_back(client);
  INFO_LOG() << "Successfully connected with client[" << client->formatedName() << "], from server["
             << formatedName() << "], " << clients_.size() << " client(s) connected.";
}

void ITcpLoop::closeClient(TcpClient* client) {
  CHECK(client->server() == this);
  loop_->stop_io(client->read_write_io_);

  if (observer_) {
    observer_->closed(client);
  }
  clients_.erase(std::remove(clients_.begin(), clients_.end(), client), clients_.end());
  INFO_LOG() << "Successfully disconnected client[" << client->formatedName() << "], from server["
             << formatedName() << "], " << clients_.size() << " client(s) connected.";
}

timer_id_t ITcpLoop::createTimer(double sec, double repeat) {
  LoopTimer* timer = new LoopTimer(this);
  ev_timer_init(timer->timer_, timer_cb, sec, repeat);
  loop_->start_timer(timer->timer_);
  timers_.push_back(timer);
  return timer->id();
}

void ITcpLoop::removeTimer(timer_id_t id) {
  for (std::vector<LoopTimer*>::iterator it = timers_.begin(); it != timers_.end(); ++it) {
    LoopTimer* timer = *it;
    if (timer->id() == id) {
      timers_.erase(it);
      delete timer;
      return;
    }
  }
}

common::patterns::id_counter<ITcpLoop>::type_t ITcpLoop::id() const {
  return id_.id();
}

void ITcpLoop::execInLoopThread(async_loop_exec_function_t func) {
  loop_->execInLoopThread(func);
}

bool ITcpLoop::isLoopThread() const {
  return loop_->isLoopThread();
}

void ITcpLoop::changeFlags(TcpClient* client) {
  if (client->flags() != client->read_write_io_->events) {
    loop_->stop_io(client->read_write_io_);
    ev_io_set(client->read_write_io_, client->fd(), client->flags());
    loop_->start_io(client->read_write_io_);
  }
}

ITcpLoop* ITcpLoop::findExistLoopByPredicate(std::function<bool(ITcpLoop*)> pred) {
  if (!pred) {
    return nullptr;
  }

  lock_t loc(g_exists_loops_mutex_);
  for (size_t i = 0; i < g_exists_loops_.size(); ++i) {
    ITcpLoop* loop = g_exists_loops_[i];
    if (loop && pred(loop)) {
      return loop;
    }
  }

  return nullptr;
}

std::vector<TcpClient*> ITcpLoop::clients() const {
  return clients_;
}

void ITcpLoop::setName(const std::string& name) {
  name_ = name;
}

std::string ITcpLoop::name() const {
  return name_;
}

std::string ITcpLoop::formatedName() const {
  return common::MemSPrintf("[%s][%s(%llu)]", name(), ClassName(), id());
}

void ITcpLoop::read_write_cb(struct ev_loop* loop, struct ev_io* watcher, int revents) {
  TcpClient* pclient = reinterpret_cast<TcpClient*>(watcher->data);
  ITcpLoop* pserver = pclient->server();
  LibEvLoop* evloop = reinterpret_cast<LibEvLoop*>(ev_userdata(loop));
  CHECK(pserver && pserver->loop_ == evloop);

  if (EV_ERROR & revents) {
    DNOTREACHED();
    return;
  }

  if (revents & EV_READ) {
    if (pserver->observer_) {
      pserver->observer_->dataReceived(pclient);
    }
  }

  if (revents & EV_WRITE) {
    if (pserver->observer_) {
      pserver->observer_->dataReadyToWrite(pclient);
    }
  }
}

void ITcpLoop::timer_cb(struct ev_loop* loop, struct ev_timer* timer, int revents) {
  LoopTimer* ptimer = reinterpret_cast<LoopTimer*>(timer->data);
  ITcpLoop* pserver = ptimer->server();
  LibEvLoop* evloop = reinterpret_cast<LibEvLoop*>(ev_userdata(loop));
  CHECK(pserver && pserver->loop_ == evloop);

  if (EV_ERROR & revents) {
    DNOTREACHED();
    return;
  }

  DCHECK(revents & EV_TIMEOUT);
  if (pserver->observer_) {
    pserver->observer_->timerEmited(pserver, ptimer->id());
  }
}

void ITcpLoop::preLooped(LibEvLoop* loop) {
  {
    lock_t loc(g_exists_loops_mutex_);
    g_exists_loops_.push_back(this);
  }

  if (observer_) {
    observer_->preLooped(this);
  }
}

void ITcpLoop::stoped(LibEvLoop* loop) {
  const std::vector<LoopTimer*> timers = timers_;
  for (size_t i = 0; i < timers.size(); ++i) {
    LoopTimer* timer = timers[i];
    removeTimer(timer->id());
  }

  const std::vector<TcpClient*> cl = clients();

  for (size_t i = 0; i < cl.size(); ++i) {
    TcpClient* client = cl[i];
    client->close();
    delete client;
  }
}

void ITcpLoop::postLooped(LibEvLoop* loop) {
  {
    lock_t loc(g_exists_loops_mutex_);
    g_exists_loops_.erase(std::remove(g_exists_loops_.begin(), g_exists_loops_.end(), this),
                          g_exists_loops_.end());
  }

  if (observer_) {
    observer_->postLooped(this);
  }
}

// server
TcpServer::TcpServer(const common::net::HostAndPort& host, ITcpLoopObserver* observer)
    : ITcpLoop(observer), sock_(host), accept_io_((struct ev_io*)calloc(1, sizeof(struct ev_io))) {
  accept_io_->data = this;
}

TcpServer::~TcpServer() {
  free(accept_io_);
  accept_io_ = NULL;
}

ITcpLoop* TcpServer::findExistServerByHost(const common::net::HostAndPort& host) {
  if (!host.isValid()) {
    return nullptr;
  }

  auto find_by_host = [host](ITcpLoop* loop) -> bool {
    TcpServer* server = dynamic_cast<TcpServer*>(loop);
    if (!server) {
      return false;
    }

    return server->host() == host;
  };

  return findExistLoopByPredicate(find_by_host);
}

TcpClient* TcpServer::createClient(const common::net::socket_info& info) {
  return new TcpClient(this, info);
}

void TcpServer::preLooped(LibEvLoop* loop) {
  int fd = sock_.fd();
  ev_io_init(accept_io_, accept_cb, fd, EV_READ);
  loop->start_io(accept_io_);
  ITcpLoop::preLooped(loop);
}

void TcpServer::postLooped(LibEvLoop* loop) {
  ITcpLoop::postLooped(loop);
}

void TcpServer::stoped(LibEvLoop* loop) {
  common::Error err = sock_.close();
  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
  }

  loop->stop_io(accept_io_);
  ITcpLoop::stoped(loop);
}

common::Error TcpServer::bind() {
  return sock_.bind();
}

common::Error TcpServer::listen(int backlog) {
  return sock_.listen(backlog);
}

const char* TcpServer::ClassName() const {
  return "TcpServer";
}

common::net::HostAndPort TcpServer::host() const {
  return sock_.host();
}

common::Error TcpServer::accept(common::net::socket_info* info) {
  return sock_.accept(info);
}

void TcpServer::accept_cb(struct ev_loop* loop, struct ev_io* watcher, int revents) {
  TcpServer* pserver = reinterpret_cast<TcpServer*>(watcher->data);
  LibEvLoop* evloop = reinterpret_cast<LibEvLoop*>(ev_userdata(loop));
  CHECK(pserver && pserver->loop_ == evloop);

  if (EV_ERROR & revents) {
    DNOTREACHED();
    return;
  }

  CHECK(watcher->fd == pserver->sock_.fd());

  common::net::socket_info sinfo;
  common::Error err = pserver->accept(&sinfo);

  if (err && err->isError()) {
    DEBUG_MSG_ERROR(err);
    return;
  }

  TcpClient* pclient = pserver->createClient(sinfo);
  pserver->registerClient(pclient);
}

ITcpLoopObserver::~ITcpLoopObserver() {}

}  // namespace tcp
}  // namespace fastotv
}  // namespace fasto
