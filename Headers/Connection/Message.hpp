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

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>

#include <boost/log/trivial.hpp>

#include "steamdatabase/protobufs/steam/steammessages_base.pb.h"

#include "DestructMonitor.hpp"
#include "Connection/Serialize.hpp"
#include "Connection/Base.hpp"
#include "EnumString.hpp"

/************************************************************************/
/*
 * The type-identifiers for the messages.
 *
 * SteamKit2 calls this "EMsg"; the values are from SteamLanguage.cs
 */

namespace SteamBot
{
	namespace Connection
	{
		namespace Message
		{
			enum class Type : uint32_t
			{
                Invalid = 0,

                Multi = 1,

                DestJobFailed = 113,

                ServiceMethod = 146,
                ServiceMethodResponse = 147,
                ServiceMethodCallFromClient = 151,

                ClientHeartBeat = 703,

                ClientChangeStatus = 716,

                ClientGamesPlayed = 742,

                ClientLogOnResponse = 751,
                ClientLoggedOff = 757,
                ClientPersonaState = 766,
                ClientFriendsList = 767,
                ClientAccountInfo = 768,
                ClientGameConnectTokens = 779,
                ClientLicenseList = 780,
                ClientVACBanStatus = 782,
                ClientUpdateGuestPassesList = 798,

                ClientClanState = 822,

                ChannelEncryptRequest = 1303,
                ChannelEncryptResponse = 1304,
                ChannelEncryptResult = 1305,

                ClientGamesPlayedWithDataBlob = 5410,

                ClientIsLimitedAccount = 5430,
                ClientEmailAddrInfo = 5456,
                ClientRequestedClientStats = 5480,

                ClientServerUnavailable = 5500,
                ClientServersAvailable = 5501,

                ClientLogon = 5514,

                ClientGetClientAppList = 5518,
                ClientGetClientAppListResponse = 5519,

                ClientWalletInfoUpdate = 5528,
                ClientUpdateMachineAuth = 5537,
                ClientUpdateMachineAuthResponse = 5538,
                ClientFriendsGroupsList = 5553,

                ClientRequestWebAPIAuthenticateUserNonce = 5585,
                ClientRequestWebAPIAuthenticateUserNonceResponse = 5586,
                ClientPlayerNicknameList = 5587,

                ClientFSOfflineMessageNotification = 7523,

                ClientConcurrentSessionsBase = 9600,
                ClientPlayingSessionState = 9600,

                ServiceMethodCallFromClientNonAuthed = 9804,
                ClientHello = 9805
			};
		}
	}
}

/************************************************************************/

#include "Modules/Connection_Internal.hpp"

/************************************************************************/

namespace magic_enum
{
	namespace customize
	{
		template <> struct enum_range<SteamBot::Connection::Message::Type>
		{
			static constexpr int min=0;
			static constexpr int max=10000;
		};
	}
}

/************************************************************************/
/*
 * A base for anything that can be serialized.
 */

namespace SteamBot
{
	namespace Connection
	{
		namespace Message
		{
			class Serializeable;
            template <typename T> class ContentSerialization;
		}
	}
}

/************************************************************************/
/*
 * Messages start with a header, but there are different types of
 * headers used by different messages.
 *
 * SteamKit2 has the headers in SteamLanguageInternal.cs
 */

namespace SteamBot
{
	namespace Connection
	{
		namespace Message
		{
			namespace Header
			{
				class Base;
				class Simple;
				class Extended;
				class ProtoBuf;
			}
		}
	}
}

/************************************************************************/
/*
 * The actual message types
 */

namespace SteamBot
{
	namespace Connection
	{
		namespace Message
		{
			class Base;
			template <Type TYPE, typename HEADER, typename CONTENT> class Message;
		}
	}
}

/************************************************************************/
/*
 * This just declares the API for serlialization, and offers some
 * helpers. Derived classes still need to do the work.
 *
 * I've added the DestructMonitor so the Connection module can wait
 * until messages ave been processed, to avoid hitting an EOF in the
 * middle of processing.
 */

class SteamBot::Connection::Message::Serializeable : public SteamBot::DestructMonitor
{
protected:
	Serializeable() =default;

public:
	virtual ~Serializeable() =default;

	virtual size_t serialize(SteamBot::Connection::Serializer&) const =0;
	virtual void deserialize(SteamBot::Connection::Deserializer&) =0;

public:
	std::vector<std::byte> serialize() const;
	void deserialize(const std::span<const std::byte>&);
};

/************************************************************************/
/*
 * The base-class for all headers. This contains the type of message,
 * so we know how to read the rest of the data.
 *
 * Note: the msgType on the wire seems to have a protobuf-flag
 * as bit 31; we don't actually keep it in the msgTyoe value
 * and just add/check it on sending/receiving.
 */

class SteamBot::Connection::Message::Header::Base : public Serializeable
{
public:
	Type msgType=Type::Invalid;

protected:
	Base(Type myType)
		: msgType(myType)
	{
	}

public:
	Base() =default;
	virtual ~Base() =default;

	virtual size_t serialize(SteamBot::Connection::Serializer&) const override;
	virtual void deserialize(SteamBot::Connection::Deserializer&) override;

public:
	static Type peekMessgeType(const std::span<const std::byte>&);
};

/************************************************************************/
/*
 * The simplest header for struct-based messages
 *
 * SteamKit2, SteamLanguageInternal.cs, MsgHdr
 */

class SteamBot::Connection::Message::Header::Simple : public Base
{
public:
	uint64_t targetJobID=UINT64_MAX;
	uint64_t sourceJobID=UINT64_MAX;

public:
	Simple(Type myType)
		: Base(myType)
	{
	}

	virtual ~Simple() =default;

	virtual size_t serialize(SteamBot::Connection::Serializer& serializer) const override;
	virtual void deserialize(SteamBot::Connection::Deserializer&) override;
};

/************************************************************************/
/*
 * A more heavyweight header for "struct"-based messages
 *
 * SteamKit2, SteamLanguageInternal.cs, ExtendedClientMsgHdr
 */

class SteamBot::Connection::Message::Header::Extended : public Base
{
public:
	uint8_t headerSize=36;		// total size, starting with the Base::msgType
	uint16_t headerVersion=2;
	uint64_t targetJobID=UINT64_MAX;
	uint64_t sourceJobID=UINT64_MAX;
	uint8_t headerCanary=239;
	uint64_t steamId=0;			// also see SteamKit2, SteamID.cs, class SteamID... which I don't understand right now. This will probably be replaced with something else.
	uint32_t sessionId=0;

public:
	Extended(Type myType)
		: Base(myType)
	{
	}

	virtual ~Extended() =default;

	virtual size_t serialize(SteamBot::Connection::Serializer&) const override;
	virtual void deserialize(SteamBot::Connection::Deserializer&) override;
};

/************************************************************************/
/*
 * A header for "protobuf" messages
 *
 * SteamKit2, SteamLanguageInternal.cs, MsgHdrProtoBuf
 *
 * steamdatabase/protobufs/steam/steammessages_base.proto:message CMsgProtoBufHeader
 */

class SteamBot::Connection::Message::Header::ProtoBuf : public Base
{
public:
	typedef uint32_t protoLengthType;	// we don't actually store it here, but it's on the wire
	CMsgProtoBufHeader proto;

private:
    void addSessionInfo();

public:
	ProtoBuf(Type);
	virtual ~ProtoBuf();

	virtual size_t serialize(SteamBot::Connection::Serializer&) const override;
	virtual void deserialize(SteamBot::Connection::Deserializer&) override;
};

/************************************************************************/
/*
 * Base class for all messages
 */

class SteamBot::Connection::Message::Base : public Serializeable
{
protected:
	Base() =default;

public:
	virtual ~Base() =default;

	virtual size_t serialize(SteamBot::Connection::Serializer&) const override =0;
	virtual void deserialize(SteamBot::Connection::Deserializer&) override =0;

public:
	// serialize and send the message
	// Note: this is an internal function. Post a SendSteamMessage to the messageboard instead of trying to call this.
	void send(Connection::Base&) const;
};

/************************************************************************/

template <typename T> class SteamBot::Connection::Message::ContentSerialization
{
public:
    inline static size_t serialize(SteamBot::Connection::Serializer& serializer, const T& content)
    {
        if constexpr (std::derived_from<T, google::protobuf::MessageLite>)
        {
            return serializer.storeProto(content);
        }
        else
        {
            return content.serialize(serializer);
        }
    }

    inline static void deserialize(SteamBot::Connection::Deserializer& deserializer, T& content)
    {
        if constexpr (std::derived_from<T, google::protobuf::MessageLite>)
        {
			deserializer.getProto(content, deserializer.data.size());
        }
        else
        {
            content.deserialize(deserializer);
        }
    }
};

/************************************************************************/
/*
 * Provide the Type enum value, header class, and content class to
 * create a message type.
 * Constructor takes parameters for the content class.
 */

template <SteamBot::Connection::Message::Type TYPE, typename HEADER, typename CONTENT> class SteamBot::Connection::Message::Message : public Base
{
	static_assert(std::derived_from<CONTENT, google::protobuf::MessageLite> == std::is_same_v<HEADER, Header::ProtoBuf>);

public:
	static constexpr auto messageType=TYPE;
	typedef CONTENT contentType;
	typedef HEADER headerType;

public:
	HEADER header;
	CONTENT content;

public:
	Message(std::span<const std::byte> bytes)
		: header(TYPE)
	{
		deserialize(bytes);
	}

	template <typename ...CONTENTPARAMS> Message(CONTENTPARAMS&& ...contentParams)
		: header(TYPE),
		  content(std::forward<CONTENTPARAMS>(contentParams)...)
	{
	}

	virtual ~Message() =default;

public:
	virtual size_t serialize(SteamBot::Connection::Serializer& serializer) const override
	{
		size_t result=header.serialize(serializer);
		result+=ContentSerialization<CONTENT>::serialize(serializer,content);
		return result;
	}

	virtual void deserialize(SteamBot::Connection::Deserializer& deserializer) override
	{
		header.deserialize(deserializer);
		ContentSerialization<CONTENT>::deserialize(deserializer, content);
        if (deserializer.data.size()>0)
        {
            BOOST_LOG_TRIVIAL(debug) << "message has " << deserializer.data.size() << " excess bytes at the end";
        }
	}

	using Serializeable::deserialize;

public:
    // This is an internal "hack" to register used message classes
    // with the Connection module, so it can deserialize them and post
    // them to the messageboard
    static void createdWaiter()
    {
        SteamBot::Modules::Connection::Internal::Handler<Message>::create();
    }
};

/************************************************************************/

#define TYPEDEF_PROTOBUF_MESSAGE(name)\
	typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::name, \
											SteamBot::Connection::Message::Header::ProtoBuf, \
											CMsg##name> CMsg##name##MessageType
