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

/************************************************************************/
/*
 * SteamKit2/Types/KeyValue.cs
 */

#include "Printable.hpp"

#include <vector>
#include <cassert>
#include <string>
#include <memory>
#include <unordered_map>
#include <span>

/************************************************************************/

namespace Steam
{
    namespace KeyValue
    {
        class ItemBase : public SteamBot::Printable
        {
        public:
            typedef std::unique_ptr<ItemBase> Ptr;

        protected:
            ItemBase() =default;

        public:
            virtual ~ItemBase() =default;
        };
    }
}

/************************************************************************/

namespace Steam
{
    namespace KeyValue
    {
        template <typename T> class Value : public ItemBase
        {
        public:
            const T value;

        public:
            template <typename U> Value(U&& value_)
                : value(std::forward<U>(value_))
            {
            }

            virtual ~Value() =default;

        private:
            virtual boost::json::value toJson() const override
            {
                return boost::json::value(value);
            }
        };
    }
}

/************************************************************************/

namespace Steam
{
    namespace KeyValue
    {
        class Node : public ItemBase
        {
        public:
            std::unordered_map<std::string, Ptr> children;

        public:
            Node() =default;
            Node(Node&&) =default;
            virtual ~Node() =default;
            virtual boost::json::value toJson() const override;

            Node& operator=(Node&&) =default;

        public:
            Node* getNode(const std::string&) const;
            Node& createNode(std::string);

        public:
            template <typename T> void setValue(std::string name, T value)
            {
                Ptr& child=children[std::move(name)];
                child.reset(new Value<T>(std::move(value)));
            }

            template <typename T> const T* getValue(const std::string& name) const
            {
                auto iterator=children.find(name);
                if (iterator!=children.end())
                {
                    auto value=dynamic_cast<Value<T>*>(iterator->second.get());
                    assert(value!=nullptr);
                    return &value->value;
                }
                return nullptr;
            }
        };
    }
}

/************************************************************************/

namespace Steam
{
    namespace KeyValue
    {
		typedef std::vector<std::byte> BinarySerializationType;
        BinarySerializationType serialize(const std::string&, const Node&);

        typedef std::span<const std::byte> BinaryDeserializationType;
        std::unique_ptr<Node> deserialize(BinaryDeserializationType, std::string&);

        // Internal use
        enum class DataType : uint8_t {
            None = 0,
            String = 1,
            Int32 = 2,
            // Float32 = 3,
            // Pointer = 4,
            // WideString = 5,
            // Color = 6,
            UInt64 = 7,
            End = 8,
            Int64 = 10,
        };
    }
}
