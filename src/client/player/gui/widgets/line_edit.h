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

#include "client/player/gui/widgets/label.h"

#include "client/player/gui/events/key_events.h"

namespace fastotv {
namespace client {
namespace player {
namespace gui {

class LineEdit : public Label {
 public:
  typedef Label base_class;

  LineEdit();
  LineEdit(const SDL_Color& back_ground_color);
  virtual ~LineEdit();

  bool IsActived() const;
  void SetActived(bool active);

  virtual void Draw(SDL_Renderer* render) override;

 protected:
  virtual void HandleEvent(event_t* event) override;
  virtual void HandleExceptionEvent(event_t* event, common::Error err) override;

  virtual void HandleMousePressEvent(player::gui::events::MousePressEvent* event) override;

  virtual void HandleKeyPressEvent(gui::events::KeyPressEvent* event);
  virtual void HandleKeyReleaseEvent(gui::events::KeyReleaseEvent* event);
  virtual void HandleTextInputEvent(gui::events::TextInputEvent* event);

  virtual void OnActiveChanged(bool active);

 private:
  void Init();
  bool active_;
};

}  // namespace gui
}  // namespace player
}  // namespace client
}  // namespace fastotv
