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

#include "client/player/gui/widgets/button.h"

#include <common/application/application.h>

namespace fastotv {
namespace client {
namespace player {
namespace gui {

Button::Button() : base_class() {
  fApp->Subscribe(this, gui::events::MouseReleaseEvent::EventType);
}

Button::Button(const SDL_Color& back_ground_color) : base_class(back_ground_color) {}

Button::~Button() {}

void Button::HandleEvent(event_t* event) {
  if (event->GetEventType() == gui::events::MouseReleaseEvent::EventType) {
    gui::events::MouseReleaseEvent* mouse_release = static_cast<gui::events::MouseReleaseEvent*>(event);
    HandleMouseReleaseEvent(mouse_release);
  }

  base_class::HandleEvent(event);
}

void Button::HandleExceptionEvent(event_t* event, common::Error err) {
  UNUSED(event);
  UNUSED(err);
}

void Button::HandleMouseReleaseEvent(events::MouseReleaseEvent* event) {
  UNUSED(event);
}

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
