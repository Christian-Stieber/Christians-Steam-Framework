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

#include <span>

#include <boost/log/trivial.hpp>
#include <boost/endian/buffers.hpp>

/************************************************************************/

typedef Steam::KeyValue::ItemBase ItemBase;
typedef Steam::KeyValue::Node Node;
typedef Steam::KeyValue::BinarySerializationType BinarySerializationType;
typedef Steam::KeyValue::DataType DataType;
template <typename T> using Value=Steam::KeyValue::Value<T>;

/************************************************************************/

typedef std::span<const std::byte> bytesType;

/************************************************************************/

namespace
{
    class ErrorException { };

    std::byte getByte(bytesType& bytes)
    {
        if (bytes.size()==0)
        {
            throw ErrorException();
        }
        auto result=bytes.front();
        bytes=bytes.last(bytes.size()-1);
        return result;
    }

    std::string getString(bytesType& bytes)
    {
        std::string result;
        while (true)
        {
            char c=static_cast<char>(getByte(bytes));
            if (c=='\0')
            {
                return result;
            }
            result.push_back(c);
        }
    }
}

/************************************************************************/

namespace
{
    template <typename T> Value<T>* getNumber(bytesType& bytes)
    {
        if (bytes.size()<sizeof(T))
        {
            throw ErrorException();
        }

        typedef boost::endian::endian_buffer<boost::endian::order::little, T, sizeof(T)*CHAR_BIT> EndianType;
        auto ptr=static_cast<const EndianType*>(static_cast<const void*>(bytes.data()));
        assert(sizeof(*ptr)==sizeof(T));
        auto result=ptr->value();
        bytes=bytes.last(bytes.size()-sizeof(T));
        return new Value<T>(result);
    }
}

/************************************************************************/

namespace
{
    void deserialize(bytesType& bytes, Node& node)
    {
        while (true)
        {
            const auto type=static_cast<DataType>(getByte(bytes));
            if (type==DataType::End)
            {
                return;
            }

            auto name=getString(bytes);
            std::unique_ptr<ItemBase> child;

            switch(type)
            {
            case DataType::None:
                {
                    auto childNode=new Node();
                    child.reset(childNode);
                    deserialize(bytes, *childNode);
                }
                break;

            case DataType::String:
                child.reset(new Value<std::string>(getString(bytes)));
                break;

            case DataType::Int32:
                child.reset(getNumber<int32_t>(bytes));
                break;

            case DataType::Int64:
                child.reset(getNumber<int64_t>(bytes));
                break;

            case DataType::UInt64:
                child.reset(getNumber<uint64_t>(bytes));
                break;

            default:
                assert(type!=DataType::End);
                throw ErrorException();
            }

            assert(child);
            node.children[std::move(name)]=std::move(child);
        }
    }
}

/************************************************************************/

std::unique_ptr<Node> Steam::KeyValue::deserialize(const BinarySerializationType& bytes_, std::string& name)
{
    std::unique_ptr<Node> result;
    try
    {
        Node fake;

        bytesType bytes(bytes_);
        ::deserialize(bytes, fake);

        assert(fake.children.size()==1);
        auto root=fake.children.begin();
        {
            auto node=dynamic_cast<Node*>(root->second.get());
            assert(node!=nullptr);
            root->second.release();
            result.reset(node);
        }
        name=root->first;

        BOOST_LOG_TRIVIAL(debug) << bytes.data()-bytes_.data() << " bytes (out of " << bytes_.size()
                                 << ") have been turned into a KeyValue tree of name \"" << name << "\": " << *result;
    }
    catch(const ErrorException&)
    {
        BOOST_LOG_TRIVIAL(debug) << "invalid KeyValue data";
    }
    return result;
}
