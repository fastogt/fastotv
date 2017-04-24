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

#include <vector>
#include <string>
#include <functional>

#include <common/patterns/crtp_pattern.h>

#include <common/net/socket_tcp.h>

#include "network/event_loop.h"

#define INVALID_TIMER_ID -1

namespace fasto {
namespace fastotv {
namespace network {
typedef int timer_id_t;
class IoLoopObserver;
class IoClient;
class LoopTimer;

class IoLoop : public EvLoopObserver, common::IMetaClassInfo {
 public:
  explicit IoLoop(IoLoopObserver* observer = nullptr);
  virtual ~IoLoop();

  int Exec() WARN_UNUSED_RESULT;
  virtual void Stop();

  void RegisterClient(const common::net::socket_info& info);
  void RegisterClient(IoClient* client);
  void UnRegisterClient(IoClient* client);
  virtual void CloseClient(IoClient* client);

  timer_id_t CreateTimer(double sec, double repeat);
  void RemoveTimer(timer_id_t id);

  void ChangeFlags(IoClient* client);

  common::patterns::id_counter<IoLoop>::type_t GetId() const;

  void SetName(const std::string& name);
  std::string Name() const;

  virtual const char* ClassName() const override = 0;
  std::string FormatedName() const;

  void ExecInLoopThread(async_loop_exec_function_t func);

  bool IsLoopThread() const;

  std::vector<IoClient*> Clients() const;

  static IoLoop* FindExistLoopByPredicate(std::function<bool(IoLoop*)> pred);

 protected:
  virtual IoClient* CreateClient(const common::net::socket_info& info) = 0;

  virtual void PreLooped(LibEvLoop* loop) override;
  virtual void Stoped(LibEvLoop* loop) override;
  virtual void PostLooped(LibEvLoop* loop) override;

  LibEvLoop* const loop_;

 private:
  static void read_write_cb(struct ev_loop* loop, struct ev_io* watcher, int revents);
  static void timer_cb(struct ev_loop* loop, struct ev_timer* timer, int revents);

  IoLoopObserver* const observer_;

  std::vector<IoClient*> clients_;
  std::vector<LoopTimer*> timers_;
  const common::patterns::id_counter<IoLoop> id_;

  std::string name_;
};

class IoLoopObserver {
 public:
  virtual void PreLooped(IoLoop* server) = 0;

  virtual void Accepted(IoClient* client) = 0;
  virtual void Moved(IoClient* client) = 0;
  virtual void Closed(IoClient* client) = 0;
  virtual void TimerEmited(IoLoop* server, timer_id_t id) = 0;

  virtual void DataReceived(IoClient* client) = 0;
  virtual void DataReadyToWrite(IoClient* client) = 0;

  virtual void PostLooped(IoLoop* server) = 0;

  virtual ~IoLoopObserver();
};
}
}  // namespace fastotv
}  // namespace fasto
