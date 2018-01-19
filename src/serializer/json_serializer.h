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

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>  // for json_tokener_parse

#include <common/serializer/iserializer.h>

namespace fastotv {

class JsonSerializer : public common::serializer::ISerializer<struct json_object*> {
 public:
  typedef common::serializer::ISerializer<struct json_object*> base_class;
  typedef typename base_class::serialize_type serialize_type;

  virtual common::Error SerializeToString(std::string* deserialized) const override final WARN_UNUSED_RESULT {
    serialize_type des = NULL;
    common::Error err = base_class::Serialize(&des);
    if (err) {
      return err;
    }

    *deserialized = json_object_get_string(des);
    json_object_put(des);
    return common::Error();
  }

  virtual common::Error SerializeFromString(const std::string& data,
                                            serialize_type* out) const override final WARN_UNUSED_RESULT {
    const char* data_ptr = data.c_str();
    serialize_type res = json_tokener_parse(data_ptr);
    if (!res) {
      return common::make_error_inval();
    }

    *out = res;
    return common::Error();
  }

 protected:
  virtual common::Error DoSerialize(serialize_type* deserialized) const override = 0;
};

class JsonSerializerEx : public JsonSerializer {
 protected:
  virtual common::Error SerializeFields(json_object* obj) const = 0;

  virtual common::Error DoSerialize(serialize_type* deserialized) const override final {
    json_object* obj = json_object_new_object();
    common::Error err = SerializeFields(obj);
    if (err) {
      json_object_put(obj);
      return err;
    }

    *deserialized = obj;
    return common::Error();
  }
};

}  // namespace fastotv
