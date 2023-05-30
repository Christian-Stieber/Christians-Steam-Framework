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

#include "Client/Connection.hpp"

#include <charconv>

/************************************************************************/
/*
 * Takes a string like 103.28.54.165:27018
 * Throws InvalidEndpointString.
 */

Steam::Client::Connection::Endpoint::Endpoint(const std::string& string)
{
	const auto index=string.rfind(':');
	if (index==std::string::npos)
	{
		throw InvalidEndpointString();
	}

	/* parse the port value in the string */
	{
		const auto last=string.data()+string.size();
		const auto result=std::from_chars(string.data()+index+1, last, port_);
		if (result.ec!=std::errc())
		{
			throw InvalidEndpointString();
		}
		if (result.ptr!=last)
		{
			throw InvalidEndpointString();
		}
	}

	/* parse the IP address */
	{
		const std::string_view view(string.data(), index);
		boost::system::error_code errorCode;
		address_=boost::asio::ip::make_address(view, errorCode);
		if (errorCode)
		{
			throw InvalidEndpointString();
		}
	}
}
