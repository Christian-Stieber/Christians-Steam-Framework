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

#pragma once

#include <boost/json.hpp>

/************************************************************************/
/*
 * Note: either the proto files that I have, or protoc, or my usage
 * thereof are messed up. webui/service_saleitemrewards.proto doesn't
 * work for me, even after adding workaround because they use a
 * "linux" field that conflicts with a stupid macro -- I end up with
 * lots of duplicate symbols.
 *
 * Trying to use protobuf and just copying the items that I need
 * didn't realy work either, thanks to CMake being a "Complicated
 * Make" again, so I gave up eventually and used ProtoPuf instead.
 */

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wshadow"
#endif

#include "External/ProtoPuf/message.h"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

/************************************************************************/

namespace SteamBot
{
    namespace ProtoPuf
    {
        template <typename T> std::vector<std::byte> encode(const T& message)
        {
            std::vector<std::byte> result;

            auto size=pp::skipper<pp::message_coder<T>>::encode_skip(message);
            result.reserve(size);
            result.resize(size);
            pp::message_coder<T>::encode(message, result);

            return result;
        }

        template <typename T> T decode(std::span<std::byte> bytes)
        {
            return std::move(pp::message_coder<T>::decode(bytes).first);
        }
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace ProtoPuf
    {
        template <typename T, pp::basic_fixed_string NAME> requires pp::is_message<T>
        decltype(auto) getItem(const T* message, pp::constant<NAME>)
        {
            typedef decltype(&(message->template get<NAME>()).value()) ResultType;
            ResultType result=nullptr;

            if (message!=nullptr)
            {
                auto& item=message->template get<NAME>();
                if (item.has_value())
                {
                    result=&item.value();
                }
            }
            return result;
        }
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace ProtoPuf
    {
        template <typename T> boost::json::object toJson(const T& message)
        {
            boost::json::object object;
            message.for_each([&object](auto&& field) {
                typedef std::remove_reference_t<decltype(field)> FieldType;

                std::string_view name=FieldType::name;
                const auto& value=field.cast_to_base();

                typedef std::remove_reference_t<decltype(value)> ValueType;
                if constexpr (FieldType::attr==pp::repeated)
                {
                    boost::json::array array;
                    for (const auto& item : value)
                    {
                        if constexpr (pp::is_message<typename ValueType::value_type>)
                        {
                            array.push_back(toJson(item));
                        }
                        else
                        {
                            array.push_back(item);
                        }
                    }
                    if (!array.empty())
                    {
                        object[name]=std::move(array);
                    }
                }
                else if constexpr (FieldType::attr==pp::singular)
                {
                    if (value.has_value())
                    {
                        if constexpr (pp::is_message<typename ValueType::value_type>)
                        {
                            object[name]=toJson(*value);
                        }
                        else
                        {
                            object[name]=*value;
                        }
                    }
                }
                else
                {
                    /*static_*/assert(false);
                }
            });
            return object;
        }
    }
}
