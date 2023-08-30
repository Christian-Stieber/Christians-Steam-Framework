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

#include "Connection/Encrypted.hpp"
#include "Connection/Message.hpp"
#include "OpenSSL/AES.hpp"
#include "OpenSSL/RSA.hpp"
#include "OpenSSL/Random.hpp"
#include "EnumString.hpp"
#include "Universe.hpp"
#include "ResultCode.hpp"

#include <boost/crc.hpp>

/************************************************************************/

typedef SteamBot::Connection::Encrypted Encrypted;

/************************************************************************/

static const uint32_t encryptionProtocolVersion=1;

/************************************************************************/

Encrypted::Encrypted(std::unique_ptr<Base> myConnection)
	: connection(std::move(myConnection))
{
}

/************************************************************************/

Encrypted::~Encrypted() =default;

/************************************************************************/

void Encrypted::connect(const Endpoint& endpoint)
{
	connection->connect(endpoint);
	establishEncryption();
}

/************************************************************************/

void Encrypted::disconnect()
{
	connection->disconnect();
}

/************************************************************************/
/*
 * SteamKit2, SteamLanguageInternal.cs, MsgChannelEncryptRequest
 */

namespace
{
	class MsgChannelEncryptRequest : public SteamBot::Connection::Message::Serializeable
	{
	public:
		typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::ChannelEncryptRequest, SteamBot::Connection::Message::Header::Simple, MsgChannelEncryptRequest> MessageType;

	public:
		uint32_t protocolVersion=encryptionProtocolVersion;
		SteamBot::Universe::Type universe=SteamBot::Universe::Type::Invalid;
		std::span<const std::byte> randomChallenge;

	public:
		virtual size_t serialize(SteamBot::Connection::Serializer&) const override
		{
			// we are not sending these
			assert(false);
			return 0;
		}

		virtual void deserialize(SteamBot::Connection::Deserializer& deserializer) override
		{
			Serializeable::deserialize(deserializer);
			deserializer.get(protocolVersion, universe);
			if (deserializer.data.size()>=16)
			{
				randomChallenge=deserializer.getRemaining();
			}
		}
	};
}

/************************************************************************/
/*
 * SteamKit2, SteamLanguageInternal.cs, MsgChannelEncryptResponse
 */

namespace
{
	class MsgChannelEncryptResponse : public SteamBot::Connection::Message::Serializeable
	{
	public:
		typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::ChannelEncryptResponse, SteamBot::Connection::Message::Header::Simple, MsgChannelEncryptResponse> MessageType;

	public:
		uint32_t protocolVersion=encryptionProtocolVersion;
		uint32_t keySize=128;
		std::vector<std::byte> encryptedHandshakeBlob;
		uint32_t keyCRC=0;

		virtual size_t serialize(SteamBot::Connection::Serializer& serializer) const override
		{
			Serializeable::serialize(serializer);
			return serializer.store(protocolVersion, keySize, encryptedHandshakeBlob, keyCRC, static_cast<uint32_t>(0));
		}

		virtual void deserialize(SteamBot::Connection::Deserializer&) override
		{
			// we are not receiving these
			assert(false);
		}
	};
}

/************************************************************************/
/*
 * SteamKit2 EnvelopeEncryptedConnection.cs, HandleEncryptRequest.
 *
 * This gets a called with a received "ChannelEncryptRequest" message.
 *
 * Sends the response.
 */

void Encrypted::handleEncryptRequest(std::span<const std::byte> bytes)
{
	MsgChannelEncryptRequest::MessageType request;
	request.deserialize(bytes);

	BOOST_LOG_TRIVIAL(debug) << "encryption request for protocol version " << request.content.protocolVersion << ", universe " << SteamBot::enumToString(request.content.universe);

	assert(request.content.protocolVersion==encryptionProtocolVersion);
	assert(request.content.universe==SteamBot::Universe::Type::Public);

	std::array<std::byte, 32> tempSessionKey;
	OpenSSL::makeRandomBytes(tempSessionKey);

	MsgChannelEncryptResponse::MessageType response;

	// RSA-encrypt the session key.
	// If the request has a randomChallenge, add it to the plaintext as well
	{
		OpenSSL::RSACrypto rsa(SteamBot::Universe::get(SteamBot::Universe::Type::Public));
		std::vector<std::byte> blobToEncrypt;
		blobToEncrypt.reserve(tempSessionKey.size()+request.content.randomChallenge.size());
		blobToEncrypt.insert(blobToEncrypt.end(), tempSessionKey.cbegin(), tempSessionKey.cend());
		blobToEncrypt.insert(blobToEncrypt.end(), request.content.randomChallenge.begin(), request.content.randomChallenge.end());
		response.content.encryptedHandshakeBlob=rsa.encrypt(blobToEncrypt);
	}

	// https://stackoverflow.com/questions/2573726/how-to-use-boostcrc
	{
		boost::crc_32_type result;
		result.process_bytes(response.content.encryptedHandshakeBlob.data(), response.content.encryptedHandshakeBlob.size());
		response.content.keyCRC=result.checksum();
	}

	assert(!encryptionEngine);
	if (request.content.randomChallenge.empty())
	{
		encryptionEngine=std::make_unique<OpenSSL::AESCrypto>(tempSessionKey);
	}
	else
	{
		encryptionEngine=std::make_unique<OpenSSL::AESCryptoHMAC>(tempSessionKey);
	}

	encryptionState=EncryptionState::Challenged;
	response.send(*connection);
}

/************************************************************************/
/*
 * SteamKit2, SteamLanguageInternal.cs, MsgChannelEncryptResult
 */

namespace
{
	class MsgChannelEncryptResult : public SteamBot::Connection::Message::Serializeable
	{
	public:
		typedef SteamBot::Connection::Message::Message<SteamBot::Connection::Message::Type::ChannelEncryptResult, SteamBot::Connection::Message::Header::Simple, MsgChannelEncryptResult> MessageType;

	public:
        SteamBot::ResultCode result=SteamBot::ResultCode::Invalid;

	public:
		virtual size_t serialize(SteamBot::Connection::Serializer&) const override
		{
			// we are not sending these
			assert(false);
			return 0;
		}

		virtual void deserialize(SteamBot::Connection::Deserializer& deserializer) override
		{
			Serializeable::deserialize(deserializer);
			deserializer.get(result);

			BOOST_LOG_TRIVIAL(debug) << "received MsgChannelEncryptResult: " << toJson();
		}

	public:
		boost::json::value toJson() const
		{
            boost::json::object json;
			SteamBot::enumToJson(json, "result", result);
			return json;
		}
	};
}

/************************************************************************/
/*
 * SteamKit2 EnvelopeEncryptedConnection.cs, HandleEncryptResult.
 *
 * This gets a called with a received "ChannelEncryptResult" message.
 */

void Encrypted::handleEncryptResult(std::span<const std::byte> bytes)
{
	MsgChannelEncryptResult::MessageType result;
	result.deserialize(bytes);

	if (result.content.result==ResultCode::OK)
	{
		assert(encryptionState!=EncryptionState::Encrypting);
		encryptionState=EncryptionState::Encrypting;
	}
	else
	{
		class EncryptionFailedException : public DataException { };
		throw EncryptionFailedException();
	}
}

/************************************************************************/
/*
 * SteamKit2 has this as part of readPacket(), but that causes a bit
 * of a problem for us
 */

void Encrypted::establishEncryption()
{
	while (encryptionState!=EncryptionState::Encrypting)
	{
		auto bytes=connection->readPacket();
		const auto msgType=Message::Header::Base::peekMessgeType(bytes);

		// SteamKit2 EnvelopeEncryptedConnection.cs checks a "Connected" state,
		// but how could we possibly get a packet without being conneted?
		if (encryptionState==EncryptionState::None && msgType==Message::Type::ChannelEncryptRequest)
		{
			handleEncryptRequest(bytes);
		}
		else if (encryptionState==EncryptionState::Challenged && msgType==Message::Type::ChannelEncryptResult)
		{
			handleEncryptResult(bytes);
		}
		else
		{
			BOOST_LOG_TRIVIAL(info) << "ignoring unexpected PacketMsg " << SteamBot::enumToString(msgType) << " before setting up encryption";
		}
	}
}

/************************************************************************/

std::span<std::byte> Encrypted::readPacket()
{
	assert(encryptionState==EncryptionState::Encrypting);

	auto bytes=connection->readPacket();
	readBuffer=encryptionEngine->decrypt(bytes);
	return readBuffer;
}

/************************************************************************/

void Encrypted::writePacket(std::span<const std::byte> bytes)
{
	assert(encryptionState==EncryptionState::Encrypting);
	connection->writePacket(encryptionEngine->encrypt(bytes));
}

/************************************************************************/

void Encrypted::getLocalAddress(Endpoint& endpoint) const
{
	return connection->getLocalAddress(endpoint);
}

/************************************************************************/

void Encrypted::cancel()
{
	connection->cancel();
}
