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

#include "OpenSSL/AES.hpp"
#include "OpenSSL/Random.hpp"
#include "./XCrypt.hpp"

/************************************************************************/

typedef SteamBot::OpenSSL::AESCrypto AESCrypto;

/************************************************************************/

std::vector<std::byte> AESCrypto::encrypt(const std::span<const std::byte>& bytes) const
{
	// create a random initialization vector
	std::array<std::byte, 16> iv;
	OpenSSL::makeRandomBytes(iv);

	return encryptWithIV(bytes, iv);
}

/************************************************************************/

std::vector<std::byte> AESCrypto::decrypt(const std::span<const std::byte>& bytes) const
{
	std::array<std::byte, 16> iv;

	return decryptWithIV(bytes, iv);
}
