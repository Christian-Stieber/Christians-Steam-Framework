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

#include "Connection/Serialize.hpp"
#include "Helpers/ProtoBuf.hpp"

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/message.h>

#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::Connection::Serializer Serializer;
typedef SteamBot::Connection::Deserializer Deserializer;

/************************************************************************/

Serializer::Serializer() =default;
Serializer::~Serializer() =default;

/************************************************************************/

Deserializer::Deserializer(SteamBot::Connection::Base::ConstBytes myData)
    : data(myData)
{
}

Deserializer::~Deserializer() =default;

/************************************************************************/
/*
 * Extracts information (name and content as json) from a protobuf
 * message. Used for debugging outputs.
 */

namespace
{
    class ProtobufDebug
    {
    public:
        std::string name;
        boost::json::value json;

        ProtobufDebug(const google::protobuf::MessageLite& protobufMessage)
        {
            auto message=dynamic_cast<const google::protobuf::Message*>(&protobufMessage);
            if (message!=nullptr)
            {
                static const std::string_view typeNameKey="__type_name__";

                json=SteamBot::toJson(*message);
                name=json.at(typeNameKey).as_string();
                json.as_object().erase(typeNameKey);
            }
        }
    };
}

/************************************************************************/

size_t Serializer::storeProto(const google::protobuf::MessageLite& protobufMessage)
{
	const auto messageSize=protobufMessage.ByteSizeLong();
	const auto currentIndex=result.size();
	result.resize(currentIndex+messageSize);
	if (!protobufMessage.SerializeToArray(result.data()+currentIndex, static_cast<int>(messageSize)))
	{
		throw std::runtime_error("error while serializing protobuf messsage");
	}

    if (!noLogging)
	{
		const ProtobufDebug info(protobufMessage);
		BOOST_LOG_TRIVIAL(debug) << "serializing protobuf message " << info.name << " into " << messageSize << " bytes: " << info.json;
	}

	return messageSize;
}

/************************************************************************/

void Deserializer::getProto(google::protobuf::MessageLite& protobufMessage, size_t messageSize)
{
	if (messageSize>data.size())
	{
		throw NotEnoughDataException();
	}

	google::protobuf::io::ArrayInputStream stream(data.data(), static_cast<int>(messageSize));
	if (!protobufMessage.ParseFromZeroCopyStream(&stream))
	{
		throw ProtobufException();
	}

	{
		const ProtobufDebug info(protobufMessage);
		BOOST_LOG_TRIVIAL(debug) << "deserialized protobuf message " << info.name << " from " << messageSize << " bytes: " << info.json;
	}

	const auto bytesRead=stream.ByteCount();
	assert(bytesRead>=0 && static_cast<size_t>(bytesRead)<=data.size());
	getBytes(static_cast<size_t>(bytesRead));
}
