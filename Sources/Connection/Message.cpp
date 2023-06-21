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

#include "Connection/Message.hpp"
#include "Modules/Login.hpp"

/************************************************************************/
/*
 * Set in the msgType on the wire
 */

static const uint32_t protoBufFlag=1<<31;

/************************************************************************/

std::vector<std::byte> SteamBot::Connection::Message::Serializeable::serialize() const
{
	SteamBot::Connection::Serializer serializer;
	serialize(serializer);
	return std::move(serializer.result);
}

/************************************************************************/

void SteamBot::Connection::Message::Serializeable::deserialize(const std::span<const std::byte>& bytes)
{
	SteamBot::Connection::Deserializer deserializer(bytes);
	deserialize(deserializer);
}

/************************************************************************/

size_t SteamBot::Connection::Message::Serializeable::serialize(SteamBot::Connection::Serializer&) const
{
	return 0;
}

/************************************************************************/

void SteamBot::Connection::Message::Serializeable::deserialize(SteamBot::Connection::Deserializer&)
{
}

/************************************************************************/

size_t SteamBot::Connection::Message::Header::Base::serialize(SteamBot::Connection::Serializer& serializer) const
{
	size_t result=Serializeable::serialize(serializer);

	auto msgTypeValue=static_cast<std::underlying_type_t<decltype(msgType)>>(msgType);
	if (dynamic_cast<const Header::ProtoBuf*>(this)!=nullptr)
	{
		msgTypeValue|=protoBufFlag;
	}
	result+=serializer.store(msgTypeValue);
	return result;
}

/************************************************************************/
/*
 * Note: as a special hack so we can "peek" at the message type
 * by deserializaing a Header::Base object, set the msgType
 * to "Invalid". This makes us skip all checks and update the
 * msgType with the received value.
 */

void SteamBot::Connection::Message::Header::Base::deserialize(SteamBot::Connection::Deserializer& deserializer)
{
	Serializeable::deserialize(deserializer);

	const auto myTypeValue=deserializer.get<std::underlying_type_t<Type>>();
	if (msgType==Type::Invalid)
	{
		msgType=static_cast<Type>(myTypeValue & ~protoBufFlag);
	}
	else
	{
		if ((dynamic_cast<Header::ProtoBuf*>(this)!=nullptr) != ((myTypeValue & protoBufFlag)!=0))
		{
			BOOST_LOG_TRIVIAL(info) << "received message type " << myTypeValue << " has incorrect protobuf indicator";
		}
		const auto myType=static_cast<Type>(myTypeValue & ~protoBufFlag);
		assert(myType==msgType);
	}
}

/************************************************************************/
/*
 * Convenience function to peek at the message type in a received packet
 */

SteamBot::Connection::Message::Type SteamBot::Connection::Message::Header::Base::peekMessgeType(const std::span<const std::byte>& message)
{
	Base base(Type::Invalid);
	base.Serializeable::deserialize(message);
	return base.msgType;
}

/************************************************************************/

size_t SteamBot::Connection::Message::Header::Simple::serialize(SteamBot::Connection::Serializer& serializer) const
{
	size_t result=Base::serialize(serializer);
	result+=serializer.store(targetJobID, sourceJobID);
	return result;
}

/************************************************************************/

void SteamBot::Connection::Message::Header::Simple::deserialize(SteamBot::Connection::Deserializer& deserializer)
{
	Base::deserialize(deserializer);
	deserializer.get(targetJobID, sourceJobID);
}

/************************************************************************/

size_t SteamBot::Connection::Message::Header::Extended::serialize(SteamBot::Connection::Serializer& serializer) const
{
	size_t result=Base::serialize(serializer);
	result+=serializer.store(headerVersion, targetJobID, sourceJobID, headerCanary, steamId, sessionId);
	return result;
}

/************************************************************************/

void SteamBot::Connection::Message::Header::Extended::deserialize(SteamBot::Connection::Deserializer& deserializer)
{
	Base::deserialize(deserializer);
	deserializer.get(headerVersion, targetJobID, sourceJobID, headerCanary, steamId, sessionId);
}

/************************************************************************/

SteamBot::Connection::Message::Header::ProtoBuf::ProtoBuf(SteamBot::Connection::Message::Type myType)
    : Base(myType)
{
    addSessionInfo();
}

/************************************************************************/

SteamBot::Connection::Message::Header::ProtoBuf::~ProtoBuf() =default;

/************************************************************************/

void SteamBot::Connection::Message::Header::ProtoBuf::addSessionInfo()
{
    auto& whiteboard=SteamBot::Client::getClient().whiteboard;

    if (auto steamId=whiteboard.has<SteamBot::Modules::Login::Whiteboard::SteamID>())
    {
        proto.set_steamid(steamId->getValue());
    }

    if (auto sessionId=whiteboard.has<SteamBot::Modules::Login::Whiteboard::ClientSessionID>())
    {
        proto.set_client_sessionid(static_cast<std::underlying_type_t<SteamBot::Modules::Login::Whiteboard::ClientSessionID>>(*sessionId));
    }
}

/************************************************************************/

size_t SteamBot::Connection::Message::Header::ProtoBuf::serialize(SteamBot::Connection::Serializer& serializer) const
{
	size_t result=Base::serialize(serializer);
	result+=serializer.store(static_cast<protoLengthType>(proto.ByteSizeLong()));
	result+=serializer.storeProto(proto);
	return result;
}

/************************************************************************/

void SteamBot::Connection::Message::Header::ProtoBuf::deserialize(SteamBot::Connection::Deserializer& deserializer)
{
	Base::deserialize(deserializer);
	const auto protoLength=deserializer.get<protoLengthType>();
	deserializer.getProto(proto, protoLength);
}

/************************************************************************/

size_t SteamBot::Connection::Message::Base::serialize(SteamBot::Connection::Serializer& serializer) const
{
	size_t result=Serializeable::serialize(serializer);
	return result;
}

/************************************************************************/

void SteamBot::Connection::Message::Base::send(Connection::Base& connection) const
{
    BOOST_LOG_TRIVIAL(info) << "writing message: " << boost::typeindex::type_id_runtime(*this).pretty_name();
	auto bytes=Serializeable::serialize();

#if 0
    {
        struct ToHex
        {
            static char convert(int number)
            {
                if (number>=0 && number<=9) return '0'+number;
                if (number>=10 && number<=15) return 'a'-10+number;
                return '?';
            }
        };

        std::string dump;
        for (std::byte c : bytes)
        {
            if (!dump.empty()) dump.push_back(' ');
            dump.append("0x");
            dump.push_back(ToHex::convert((static_cast<unsigned char>(c)) >> 4));
            dump.push_back(ToHex::convert((static_cast<unsigned char>(c)) & 0x0f));
        }
        BOOST_LOG_TRIVIAL(debug) << "serialized message: " << dump;
    }
#endif

	connection.writePacket(bytes);
}
