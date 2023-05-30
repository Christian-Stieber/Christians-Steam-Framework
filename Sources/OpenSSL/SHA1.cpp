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

#include "OpenSSL/SHA1.hpp"

#include <openssl/sha.h>

/************************************************************************/
/*
 * Generate SHA1 hash for some data
 */

void SteamBot::OpenSSL::calculateSHA1(std::span<const std::byte> data, std::span<std::byte, SHA_DIGEST_LENGTH> result)
{
	SHA1(static_cast<const unsigned char*>(static_cast<const void*>(data.data())), data.size(),
		 static_cast<unsigned char*>(static_cast<void*>(result.data())));
}
