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

#include "client/player/gui/label.h"

#include "client/player/gui/events/key_events.h"

namespace fastotv {
namespace client {
namespace player {
namespace gui {

class KeyPressLabel : public Label {
 public:
  typedef Label base_class;

  KeyPressLabel();
  virtual ~KeyPressLabel();

 protected:
  virtual void HandleEvent(event_t* event) override;
  virtual void HandleExceptionEvent(event_t* event, common::Error err) override;

  virtual void HandleKeyPressEvent(player::gui::events::KeyPressEvent* event);
  virtual void HandleKeyReleaseEvent(player::gui::events::KeyReleaseEvent* event);
};

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
