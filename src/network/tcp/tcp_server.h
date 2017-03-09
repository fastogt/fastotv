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

#include <vector>
#include <string>
#include <functional>

#include <common/patterns/crtp_pattern.h>

#include <common/net/socket_tcp.h>

#include "network/event_loop.h"

#define INVALID_TIMER_ID -1

namespace fasto {
namespace fastotv {

typedef int timer_id_t;

namespace tcp {

class ITcpLoopObserver;
class TcpClient;
class LoopTimer;

class ITcpLoop : public EvLoopObserver, common::IMetaClassInfo {
 public:
  explicit ITcpLoop(ITcpLoopObserver* observer = nullptr);
  virtual ~ITcpLoop();

  int exec() WARN_UNUSED_RESULT;
  virtual void stop();

  void registerClient(const common::net::socket_info& info);
  void registerClient(TcpClient* client);
  void unregisterClient(TcpClient* client);
  virtual void closeClient(TcpClient* client);

  timer_id_t createTimer(double sec, double repeat);
  void removeTimer(timer_id_t id);

  void changeFlags(TcpClient* client);

  common::patterns::id_counter<ITcpLoop>::type_t id() const;

  void setName(const std::string& name);
  std::string name() const;

  virtual const char* ClassName() const override = 0;
  std::string formatedName() const;

  void execInLoopThread(async_loop_exec_function_t func);

  bool isLoopThread() const;

  std::vector<TcpClient*> clients() const;

  static ITcpLoop* findExistLoopByPredicate(std::function<bool(ITcpLoop*)> pred);

 protected:
  virtual TcpClient* createClient(const common::net::socket_info& info) = 0;

  virtual void preLooped(LibEvLoop* loop) override;
  virtual void stoped(LibEvLoop* loop) override;
  virtual void postLooped(LibEvLoop* loop) override;

  LibEvLoop* const loop_;

 private:
  static void read_write_cb(struct ev_loop* loop, struct ev_io* watcher, int revents);
  static void timer_cb(struct ev_loop* loop, struct ev_timer* timer, int revents);

  ITcpLoopObserver* const observer_;

  std::vector<TcpClient*> clients_;
  std::vector<LoopTimer*> timers_;
  const common::patterns::id_counter<ITcpLoop> id_;

  std::string name_;
};

class ITcpLoopObserver {
 public:
  virtual void preLooped(ITcpLoop* server) = 0;

  virtual void accepted(TcpClient* client) = 0;
  virtual void moved(TcpClient* client) = 0;
  virtual void closed(TcpClient* client) = 0;
  virtual void timerEmited(ITcpLoop* server, timer_id_t id) = 0;

  virtual void dataReceived(TcpClient* client) = 0;
  virtual void dataReadyToWrite(TcpClient* client) = 0;

  virtual void postLooped(ITcpLoop* server) = 0;

  virtual ~ITcpLoopObserver();
};

class TcpServer : public ITcpLoop {
 public:
  explicit TcpServer(const common::net::HostAndPort& host, ITcpLoopObserver* observer = nullptr);
  virtual ~TcpServer();

  common::Error bind() WARN_UNUSED_RESULT;
  common::Error listen(int backlog) WARN_UNUSED_RESULT;

  const char* ClassName() const override;
  common::net::HostAndPort host() const;

  static ITcpLoop* findExistServerByHost(const common::net::HostAndPort& host);

 private:
  virtual TcpClient* createClient(const common::net::socket_info& info) override;
  virtual void preLooped(LibEvLoop* loop) override;
  virtual void postLooped(LibEvLoop* loop) override;

  virtual void stoped(LibEvLoop* loop) override;

  static void accept_cb(struct ev_loop* loop, struct ev_io* watcher, int revents);

  common::Error accept(common::net::socket_info* info) WARN_UNUSED_RESULT;

  common::net::ServerSocketTcp sock_;
  ev_io* accept_io_;
};

}  // namespace tcp
}  // namespace fastotv
}  // namespace fasto
