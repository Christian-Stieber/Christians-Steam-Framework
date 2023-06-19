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

#include "OpenSSL/Random.hpp"
#include "OpenSSL/Exception.hpp"

#include <openssl/rand.h>

/************************************************************************/
/*
 * Fill the memory with random bytes
 */

void SteamBot::OpenSSL::makeRandomBytes(std::span<std::byte> buffer)
{
    uint8_t* const bufferData=static_cast<uint8_t*>(static_cast<void*>(buffer.data()));
	Exception::throwMaybe(RAND_bytes(bufferData, static_cast<int>(buffer.size())));
}
