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

#include <common/error.h>

namespace fasto {
namespace fastotv {

template <typename T, typename S = std::string>
class ISerializer {
 public:
  typedef T value_type;
  typedef S serialize_type;

 protected:
  common::Error Serialize(serialize_type* deserialized) const WARN_UNUSED_RESULT {
    return static_cast<const T*>(this)->Serialize(deserialized);
  }
  static common::Error DeSerialize(const serialize_type& serialized, T* obj) WARN_UNUSED_RESULT {
    if (!obj) {
      return common::make_error_value("Invalid input argument(s)", common::Value::E_ERROR);
    }
    return T::DeSerialize(serialized, obj);
  }
};
}  // namespace fastotv
}  // namespace fasto
