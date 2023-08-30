/*
 * This file is part of "Christians-Steam-Framework"
 * Copyright (C) 2023- Christian Stieber
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.  If not see
 * <http://www.gnu.org/licenses/>.
 */

#include "Helpers/ProtoBuf.hpp"

#include <google/protobuf/util/json_util.h>

/************************************************************************/

boost::json::value SteamBot::toJson(const google::protobuf::Message& message, bool full)
{
    google::protobuf::util::JsonPrintOptions options;
    options.always_print_primitive_fields=full;
    options.always_print_enums_as_ints=true;
    options.preserve_proto_field_names=true;

    std::string jsonString;
    if (google::protobuf::util::MessageToJsonString(message, &jsonString, options)!=google::protobuf::util::OkStatus())
    {
        throw std::runtime_error("unable to turn protobuf into json");
    }
    auto value=boost::json::parse(jsonString);
    value.as_object()["__type_name__"]=message.GetTypeName();
    return value;
}
