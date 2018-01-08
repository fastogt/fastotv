/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

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

#include <memory>

#include <common/libev/io_loop.h>           // for IoLoop
#include <common/libev/io_loop_observer.h>  // for IoLoopObserver
#include <common/libev/loop_controller.h>   // for ILoopController

#include "chat_message.h"
#include "client_server_types.h"

namespace common {
namespace threads {
template <typename RT>
class Thread;
}
}  // namespace common

namespace fastotv {
namespace client {

class IoService : public common::libev::ILoopController {
 public:
  IoService();
  virtual ~IoService();

  bool IsRunning() const;
  void Start();
  void Stop();

  void ConnectToServer() const;
  void DisconnectFromServer() const;
  void RequestServerInfo() const;
  void RequestChannels() const;
  void RequesRuntimeChannelInfo(stream_id sid) const;
  void PostMessageToChat(const ChatMessage& msg) const;

 private:
  using ILoopController::Exec;

  virtual common::libev::IoLoopObserver* CreateHandler() override;
  virtual common::libev::IoLoop* CreateServer(common::libev::IoLoopObserver* handler) override;

  virtual void HandleStarted() override;
  virtual void HandleStoped() override;

  std::shared_ptr<common::threads::Thread<int>> loop_thread_;
};

}  // namespace client
}  // namespace fastotv
