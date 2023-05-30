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

#include "Connection/Endpoint.hpp"

#include <charconv>

/************************************************************************/

typedef SteamBot::Connection::Endpoint Endpoint;

/************************************************************************/

Endpoint::Endpoint() =default;
Endpoint::~Endpoint() =default;

/************************************************************************/

Endpoint::Endpoint(const boost::asio::ip::tcp::endpoint& other)
    : address(other.address()),
      port(other.port())
{
}

/************************************************************************/

Endpoint::Endpoint(const boost::asio::ip::udp::endpoint& other)
    : address(other.address()),
      port(other.port())
{
}

/************************************************************************/

Endpoint::operator boost::asio::ip::tcp::endpoint() const
{
    return boost::asio::ip::tcp::endpoint(address, port);
}

/************************************************************************/

Endpoint::operator boost::asio::ip::udp::endpoint() const
{
    return boost::asio::ip::udp::endpoint(address, port);
}

/************************************************************************/
/*
 * Takes a string like 103.28.54.165:27018
 * Throws InvalidStringException.
 */

Endpoint::Endpoint(std::string_view string)
{
	const auto index=string.rfind(':');
	if (index==std::string::npos)
	{
		throw InvalidStringException();
	}

	/* parse the port value in the string */
	{
		const auto last=string.data()+string.size();
		const auto result=std::from_chars(string.data()+index+1, last, port);
		if (result.ec!=std::errc())
		{
			throw InvalidStringException();
		}
		if (result.ptr!=last)
		{
			throw InvalidStringException();
		}
	}

	/* parse the IP address */
	{
		const std::string_view view(string.data(), index);
		boost::system::error_code errorCode;
		address=boost::asio::ip::make_address(view, errorCode);
		if (errorCode)
		{
			throw InvalidStringException();
		}
	}
}

/************************************************************************/

Endpoint::Endpoint(const std::string& string)
    : Endpoint(static_cast<std::string_view>(string))
{
}
