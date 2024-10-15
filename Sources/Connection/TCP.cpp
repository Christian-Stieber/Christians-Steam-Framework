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

#include "Asio/Asio.hpp"
#include "Connection/TCP.hpp"
#include "Connection/Serialize.hpp"
#include "Asio/Fiber.hpp"

#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::Connection::TCP TCP;

/************************************************************************/

namespace
{
    class PacketHeader
    {
    public:
        class InvalidMagicValueException : public SteamBot::Connection::DataException { };

    public:
		static constexpr size_t headerSize=4+4;
		static constexpr std::byte magicValue[4]={
			static_cast<std::byte>('V'),
			static_cast<std::byte>('T'),
			static_cast<std::byte>('0'),
			static_cast<std::byte>('1')
		};

    public:
		uint32_t length=0;			/* payload length */

	public:
		PacketHeader(size_t myLength)
			: length(static_cast<uint32_t>(myLength))
		{
		}

    public:
		PacketHeader(TCP::ConstBytes data)
        {
            SteamBot::Connection::Deserializer deserializer(data);

            deserializer.get(length);

            const auto magicBytes=deserializer.getBytes(sizeof(magicValue));
            if (memcmp(magicValue, magicBytes.data(), sizeof(magicValue))!=0)
            {
                throw InvalidMagicValueException();
            }

            assert(deserializer.data.size()==0);

            // BOOST_LOG_TRIVIAL(debug) << "TCP: got packet header for length " << length;
        }

    public:
		std::vector<std::byte> serialize() const
        {
            SteamBot::Connection::Serializer serializer;
            serializer.store(length);
            serializer.store(std::span<const std::byte, sizeof(magicValue)>(magicValue, sizeof(magicValue)));
            assert(serializer.result.size()==headerSize);
            return std::move(serializer.result);
        }
	};
}

/************************************************************************/

TCP::TCP()
	: socket(SteamBot::Asio::getIoContext())
{
}

/************************************************************************/

TCP::~TCP() =default;

/************************************************************************/

void TCP::connect(const Endpoint& endpoint)
{
    BOOST_LOG_TRIVIAL(debug) << "TCP: connecting to " << endpoint.address <<  ":" << endpoint.port;
    socket.async_connect(endpoint, boost::fibers::asio::yield);
}

/************************************************************************/
/*
 * Note: we can't do anything about errors anyway, so we're ignoring
 * them.
 */

void TCP::disconnect()
{
	boost::system::error_code error;
	socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
	socket.close(error);
}

/************************************************************************/

TCP::MutableBytes TCP::readPacket()
{
	std::array<std::byte, PacketHeader::headerSize> headerBytes;
	auto bytesRead=boost::asio::async_read(socket, boost::asio::buffer(headerBytes), boost::fibers::asio::yield);
	assert(bytesRead==headerBytes.size());

	PacketHeader header(headerBytes);
	readBuffer.resize(header.length);
	if (header.length>0)
	{
		/*auto bytesRead=*/ boost::asio::async_read(socket, boost::asio::buffer(readBuffer), boost::fibers::asio::yield);
		// BOOST_LOG_TRIVIAL(debug) << "TCP: read " << bytesRead << " data bytes";
	}

	return readBuffer;
}

/************************************************************************/

void TCP::writePacket(TCP::ConstBytes bytes)
{
    auto header=PacketHeader(bytes.size()).serialize();
    std::array<boost::asio::const_buffer, 2> buffers={ boost::asio::buffer(header), boost::asio::const_buffer(bytes.data(), bytes.size()) };
    // BOOST_LOG_TRIVIAL(debug) << "writing packet of " << bytes.size() << " bytes";
    boost::asio::async_write(socket, buffers, boost::fibers::asio::yield);
}

/************************************************************************/

void TCP::getLocalAddress(Endpoint& endpoint) const
{
	endpoint=socket.local_endpoint();
}

/************************************************************************/

void TCP::cancel()
{
	boost::system::error_code ec;
	socket.close(ec);
}
