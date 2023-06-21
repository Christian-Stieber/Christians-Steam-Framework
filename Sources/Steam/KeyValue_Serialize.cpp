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

#include "Steam/KeyValue.hpp"

#include <boost/log/trivial.hpp>

/************************************************************************/
/*
 * The fileformat for binary serialization seems to be like this:
 *  - a "type byte
 *  - followed by data
 * If the type is "None", it's a parent node. Children will be
 * written out, then an "end" type byte will be written
 */

/************************************************************************/

typedef Steam::KeyValue::ItemBase ItemBase;
typedef Steam::KeyValue::Node Node;
typedef Steam::KeyValue::BinarySerializationType BinarySerializationType;
typedef Steam::KeyValue::DataType DataType;
template <typename T> using Value=Steam::KeyValue::Value<T>;

/************************************************************************/

namespace
{
    template <typename T> bool serializeToBinary(const std::string&, const ItemBase*, BinarySerializationType&)
    {
        static_assert(sizeof(T*)==0, "serialization of T not supported");
        return false;
    }

    template <> bool serializeToBinary<std::string>(const std::string&, const ItemBase*, BinarySerializationType&);
    template <> bool serializeToBinary<Node>(const std::string&, const ItemBase*, BinarySerializationType&);
}

/************************************************************************/

namespace
{
    void serializeToBinary(BinarySerializationType& bytes, DataType type)
    {
        bytes.push_back(static_cast<std::byte>(static_cast<std::underlying_type_t<DataType>>(type)));
    }

    void serializeToBinary(BinarySerializationType& bytes, const std::string& string)
    {
        // we assume it's already UTF-8
        for (char c : string)
        {
            bytes.push_back(static_cast<std::byte>(c));
        }
        bytes.push_back(static_cast<std::byte>('\0'));
    }

    void serializeToBinary(BinarySerializationType& bytes, const std::string& name, const ItemBase* item)
    {
        bool success=false;
        success=success || serializeToBinary<std::string>(name, item, bytes);
        success=success || serializeToBinary<Node>(name, item, bytes);
        assert(success);
    }
}

/************************************************************************/
/*
 * Note: SteamKit doesn't support much in terms of serialization
 */

namespace
{
    template <> bool serializeToBinary<std::string>(const std::string& name, const ItemBase* item, BinarySerializationType& bytes)
    {
        if (auto value=dynamic_cast<const Value<std::string>*>(item))
        {
            serializeToBinary(bytes, DataType::String);
            serializeToBinary(bytes, name);
            serializeToBinary(bytes, value->value);
            return true;
        }
        return false;
    }

    template <> bool serializeToBinary<Node>(const std::string& name, const ItemBase* item, BinarySerializationType& bytes)
    {
        if (auto node=dynamic_cast<const Node*>(item))
        {
            serializeToBinary(bytes, DataType::None);
            serializeToBinary(bytes, name);
            for (const auto& child : node->children)
            {
                serializeToBinary(bytes, child.first, child.second.get());
            }
            serializeToBinary(bytes, DataType::End);

            // SteamKit writes TWO end markers?
            serializeToBinary(bytes, DataType::End);

            return true;
        }
        return false;
    }
}

/************************************************************************/

BinarySerializationType Steam::KeyValue::serialize(const std::string& name, const Node& node)
{
    BinarySerializationType result;
    serializeToBinary(result, name, &node);
    BOOST_LOG_TRIVIAL(debug) << "serialized KeyValue tree " << node << " into " << result.size() << " bytes";
    return result;
}
