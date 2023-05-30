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

#include <span>
#include <vector>
#include <exception>
#include <memory>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>

#define BOOST_ALLOW_DEPRECATED_HEADERS
#include <boost/asio/spawn.hpp>
#undef BOOST_ALLOW_DEPRECATED_HEADERS

#include "MultiChar.hpp"
#include "Universe.hpp"
#include "Client/OpenSSL.hpp"

#include <nlohmann/json.hpp>

/************************************************************************/

namespace Steam
{
	namespace Client
	{
		class Client;
	}
}

/************************************************************************/
/*
 * Steam can do connections via TCP, UDP, or WebSockets.  Eventually,
 * we'll want to support every type.
 *
 * Steam deals with full packets containing a length; our API only
 * works on full packets.
 *
 * Notes:
 *   - Steam uses little-endian for data transmissions
 *   - Rapsi/Debian has a very outdated g++ and boost, so I don't
 *     trust C++ coroutines at all.
 *   - instead, I'll be trying to use the boost coroutines
 *     https://theboostcpplibraries.com/boost.asio-coroutines
 *   - which means I'll also try to use exceptions instead of
 *     error codes
 *   - also, it seems I won't need shared_ptr to keep connection
 *     objects alive... let's just hope this works out :-)
 */

/************************************************************************/

namespace Steam
{
	namespace Client
	{
		namespace Connection
		{
			class Endpoint;

			class Base;
			class TCP;

			class Encrypted;
		}
	}
}

/************************************************************************/
/*
 * A "Connection::DataException" is thrown when we don't like something
 * that was received.
 */

namespace Steam
{
	namespace Client
	{
		namespace Connection
		{
			class DataException : public std::exception
			{
			private:
				const char* message;

			public:
				DataException(const char* myMessage)
					: message(myMessage)
				{
				}

				virtual const char* what() const noexcept
				{
					return message;
				}

				virtual ~DataException() =default;
			};
		}
	}
}

/************************************************************************/
/*
 * Unfortunately, boost has distinct endpoint classes for UDP and TCP.
 */

class Steam::Client::Connection::Endpoint
{
private:
	boost::asio::ip::address address_;
	uint16_t port_=0;

public:
	Endpoint() =default;

	Endpoint(const boost::asio::ip::tcp::endpoint& other)
		: address_(other.address()),
		  port_(other.port())
	{
	}

	Endpoint(const boost::asio::ip::udp::endpoint& other)
		: address_(other.address()),
		  port_(other.port())
	{
	}

public:
	class InvalidEndpointString
	{
	};

	Endpoint(const std::string&);

public:
	boost::asio::ip::address address() const
	{
		return address_;
	}

	uint16_t port() const
	{
		return port_;
	}

public:
	operator boost::asio::ip::tcp::endpoint() const
	{
		return boost::asio::ip::tcp::endpoint(address_, port_);
	}

	operator boost::asio::ip::udp::endpoint() const
	{
		return boost::asio::ip::udp::endpoint(address_, port_);
	}
};

/************************************************************************/
/*
 * This is an abstract base that acts as an interface; it's bascially
 * the IConnection of SteamKit2.
 */

class Steam::Client::Connection::Base
{
public:
	virtual ~Base() =default;

public:
	virtual void connect(const Endpoint&, boost::asio::yield_context) =0;
	virtual void disconnect(boost::asio::yield_context) =0;
	virtual std::span<std::byte> readPacket(boost::asio::yield_context) =0;
	virtual void writePacket(boost::asio::yield_context, std::span<const std::byte>) =0;

	virtual void getLocalAddress(Endpoint&) const =0;

public:
	virtual void cancel() =0;
};

/************************************************************************/
/*
 * The TCP connection. It's the easiest one to implement, so I'll
 * start with that...
 */

class Steam::Client::Connection::TCP : public Steam::Client::Connection::Base
{
private:
	class PacketHeader
	{
	public:
		class InvalidMagicValue : public DataException
		{
		public:
			InvalidMagicValue()
				: DataException("Invalid magic value in packet header")
			{
			}

			virtual ~InvalidMagicValue() =default;
		};

	public:
		static constexpr size_t headerSize=4+4;
		static constexpr std::byte magicValue[4]={
			static_cast<std::byte>('V'),
			static_cast<std::byte>('T'),
			static_cast<std::byte>('0'),
			static_cast<std::byte>('1')
		};

		uint32_t length=0;			/* payload length */

	public:
		PacketHeader(size_t myLength)
			: length(myLength)
		{
		}

		PacketHeader(const std::span<std::byte, headerSize>&);

		std::vector<std::byte> serialize() const;

	public:
		nlohmann::json toJson() const;
	};

private:
	boost::asio::ip::tcp::socket socket;

	std::vector<std::byte> readBuffer;

public:
	TCP();
	virtual ~TCP();

public:
	virtual void connect(const Endpoint&, boost::asio::yield_context) override;
	virtual void disconnect(boost::asio::yield_context) override;
	virtual std::span<std::byte> readPacket(boost::asio::yield_context) override;
	virtual void writePacket(boost::asio::yield_context, std::span<const std::byte>) override;

	virtual void getLocalAddress(Endpoint&) const override;

	virtual void cancel() override;
};

/************************************************************************/
/*
 * Encrypted connections use another connection to actually
 * send/receive data.
 *
 * In SteamKit2, these are in EnvelopeEncryptedConnection.cs
 */

class Steam::Client::Connection::Encrypted : public Steam::Client::Connection::Base
{
private:
	Steam::Client::Client& client;

	/* the transport-connection */
	const std::unique_ptr<Base> connection;

	/* encryption state */
	std::unique_ptr<OpenSSL::AESCryptoBase> encryptionEngine;
	enum class EncryptionState { None, Challenged, Encrypting } encryptionState=EncryptionState::None;

	std::vector<std::byte> readBuffer;	// since readPacket() only returns an std::span... maybe not the best idea

private:
	void handleEncryptRequest(boost::asio::yield_context, const std::span<const std::byte>&);
	void handleEncryptResult(const std::span<const std::byte>&);
	void establishEncryption(boost::asio::yield_context);

public:
	Encrypted(Steam::Client::Client&, std::unique_ptr<Base>);
	virtual ~Encrypted();

public:
	virtual void connect(const Endpoint&, boost::asio::yield_context) override;
	virtual void disconnect(boost::asio::yield_context) override;
	virtual std::span<std::byte> readPacket(boost::asio::yield_context) override;
	virtual void writePacket(boost::asio::yield_context, std::span<const std::byte>) override;

	virtual void getLocalAddress(Endpoint&) const override;

	virtual void cancel() override;
};
