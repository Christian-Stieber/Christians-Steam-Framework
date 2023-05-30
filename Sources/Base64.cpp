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
