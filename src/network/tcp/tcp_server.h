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

  void RegisterClient(const common::net::socket_info& info);
  void RegisterClient(TcpClient* client);
  void UnRegisterClient(TcpClient* client);
  virtual void CloseClient(TcpClient* client);

  timer_id_t CreateTimer(double sec, double repeat);
  void RemoveTimer(timer_id_t id);

  void ChangeFlags(TcpClient* client);

  common::patterns::id_counter<ITcpLoop>::type_t id() const;

  void SetName(const std::string& name);
  std::string Name() const;

  virtual const char* ClassName() const override = 0;
  std::string FormatedName() const;

  void ExecInLoopThread(async_loop_exec_function_t func);

  bool IsLoopThread() const;

  std::vector<TcpClient*> Clients() const;

  static ITcpLoop* FindExistLoopByPredicate(std::function<bool(ITcpLoop*)> pred);

 protected:
  virtual TcpClient* CreateClient(const common::net::socket_info& info) = 0;

  virtual void PreLooped(LibEvLoop* loop) override;
  virtual void Stoped(LibEvLoop* loop) override;
  virtual void PostLooped(LibEvLoop* loop) override;

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
  virtual void PreLooped(ITcpLoop* server) = 0;

  virtual void Accepted(TcpClient* client) = 0;
  virtual void Moved(TcpClient* client) = 0;
  virtual void Closed(TcpClient* client) = 0;
  virtual void TimerEmited(ITcpLoop* server, timer_id_t id) = 0;

  virtual void DataReceived(TcpClient* client) = 0;
  virtual void DataReadyToWrite(TcpClient* client) = 0;

  virtual void PostLooped(ITcpLoop* server) = 0;

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
  virtual TcpClient* CreateClient(const common::net::socket_info& info) override;
  virtual void PreLooped(LibEvLoop* loop) override;
  virtual void PostLooped(LibEvLoop* loop) override;

  virtual void Stoped(LibEvLoop* loop) override;

  static void accept_cb(struct ev_loop* loop, struct ev_io* watcher, int revents);

  common::Error accept(common::net::socket_info* info) WARN_UNUSED_RESULT;

  common::net::ServerSocketTcp sock_;
  ev_io* accept_io_;
};

}  // namespace tcp
}  // namespace fastotv
}  // namespace fasto
