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
