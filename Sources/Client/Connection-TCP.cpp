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

#include <cassert>

#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/log/trivial.hpp>

#include "Client/Connection.hpp"
#include "Client/Serialize.hpp"
#include "EventLoop.hpp"

/************************************************************************/

Steam::Client::Connection::TCP::TCP()
	: socket(SteamBot::getEventLoop().get_executor())
{
}

/************************************************************************/

Steam::Client::Connection::TCP::~TCP() =default;

/************************************************************************/

void Steam::Client::Connection::TCP::connect(const Endpoint& endpoint, boost::asio::yield_context context)
{
	socket.async_connect(endpoint, context);
}

/************************************************************************/
/*
 * Note: we can't do anything about errors anyway, so we're ignoring
 * them.
 *
 * This is not actually async, but I'm taking a context just for
 * consistency.
 */

void Steam::Client::Connection::TCP::disconnect(boost::asio::yield_context)
{
	boost::system::error_code error;
	socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
	socket.close(error);
}

/************************************************************************/

Steam::Client::Connection::TCP::PacketHeader::PacketHeader(const std::span<std::byte, headerSize>& data)
{
	Deserializer deserializer(data);

	deserializer.get(length);

	const auto magicBytes=deserializer.getBytes(sizeof(magicValue));
	if (memcmp(magicValue, magicBytes.data(), sizeof(magicValue))!=0)
	{
		throw InvalidMagicValue();
	}

	assert(deserializer.data.size()==0);

	BOOST_LOG_TRIVIAL(debug) << "got packet header: " << toJson();
}

/************************************************************************/

nlohmann::json Steam::Client::Connection::TCP::PacketHeader::toJson() const
{
	nlohmann::json json={
			{ "length", length }
	};
	return json;
}

/************************************************************************/
/*
 * Note: the header size is fixed so we could just serialize into a
 * fixed array... but the Serializer class can't do that right now.
 */

std::vector<std::byte> Steam::Client::Connection::TCP::PacketHeader::serialize() const
{
	Serializer serializer;
	serializer.store(length);
	serializer.store(std::span<const std::byte, sizeof(magicValue)>(magicValue, sizeof(magicValue)));
	assert(serializer.result.size()==headerSize);
	return std::move(serializer.result);
}

/************************************************************************/

std::span<std::byte> Steam::Client::Connection::TCP::readPacket(boost::asio::yield_context context)
{
	std::array<std::byte, PacketHeader::headerSize> headerBytes;
	auto bytesRead=boost::asio::async_read(socket, boost::asio::buffer(headerBytes), context);
	assert(bytesRead==headerBytes.size());

	PacketHeader header(headerBytes);
	readBuffer.resize(header.length);
	if (header.length>0)
	{
		auto bytesRead=boost::asio::async_read(socket, boost::asio::buffer(readBuffer), context);
		BOOST_LOG_TRIVIAL(debug) << "read " << bytesRead << " data bytes";
	}

	return readBuffer;
}

/************************************************************************/

void Steam::Client::Connection::TCP::writePacket(boost::asio::yield_context context, std::span<const std::byte> bytes)
{
	auto header=PacketHeader(bytes.size()).serialize();
	std::array<boost::asio::const_buffer, 2> buffers={ boost::asio::buffer(header), boost::asio::const_buffer(bytes.data(), bytes.size()) };
	BOOST_LOG_TRIVIAL(debug) << "writing packet of " << bytes.size() << " bytes";
	boost::asio::async_write(socket, buffers, context);
}

/************************************************************************/

void Steam::Client::Connection::TCP::getLocalAddress(Endpoint& endpoint) const
{
	endpoint=socket.local_endpoint();
}

/************************************************************************/

void Steam::Client::Connection::TCP::cancel()
{
	boost::system::error_code ec;
	socket.close(ec);
}
