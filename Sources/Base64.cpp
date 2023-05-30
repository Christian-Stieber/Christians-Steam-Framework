#include "Base64.hpp"

#include <boost/beast/core/detail/base64.hpp>

/************************************************************************/

std::string SteamBot::Base64::encode(std::span<const std::byte> bytes)
{
	// note: std::string reserve() is pointless -- we can change the
	// capacity, but there's no way to resize the string without
	// initializing the new characters. So we can just as well use
	// resize() instead.

	std::string result;
	result.resize(boost::beast::detail::base64::encoded_size(bytes.size()));
	result.resize(boost::beast::detail::base64::encode(result.data(), bytes.data(), bytes.size()));
	return result;
}

/************************************************************************/

std::vector<std::byte> SteamBot::Base64::decode(const std::string& string)
{
	std::vector<std::byte> result;
	result.resize(boost::beast::detail::base64::decoded_size(string.size()));
	auto info=boost::beast::detail::base64::decode(result.data(), string.data(), string.size());
	result.resize(info.first);
	return result;
}
