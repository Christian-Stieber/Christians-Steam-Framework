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

#include "OpenSSL/RSA.hpp"
#include "OpenSSL/Exception.hpp"

#include <openssl/x509.h>
#include <openssl/rsa.h>
#include <cassert>

/************************************************************************/

typedef SteamBot::OpenSSL::RSACrypto RSACrypto;

/************************************************************************/

RSACrypto::RSACrypto(const SteamBot::Universe& universe)
{
	/* load the public key */
	{
		const uint8_t* key=static_cast<const uint8_t*>(static_cast<const void*>(universe.publicKey.data()));
		const uint8_t* bytes=key;
		pkey=Exception::throwMaybe(d2i_PUBKEY(nullptr, &bytes, universe.publicKey.size()));
		assert(bytes==key+universe.publicKey.size());
	}

	/* create the context */
	{
		ctx=Exception::throwMaybe(EVP_PKEY_CTX_new(pkey, nullptr));
	}
}

/************************************************************************/

RSACrypto::~RSACrypto()
{
	EVP_PKEY_CTX_free(ctx);
	EVP_PKEY_free(pkey);
}

/************************************************************************/

std::vector<std::byte> RSACrypto::encrypt(const std::span<const std::byte>& buffer)
{
	Exception::throwMaybe(EVP_PKEY_encrypt_init(ctx));
	Exception::throwMaybe(EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING));

    const uint8_t* const bufferData=static_cast<const uint8_t*>(static_cast<const void*>(buffer.data()));

    std::size_t length;
	Exception::throwMaybe(EVP_PKEY_encrypt(ctx, nullptr, &length, bufferData, buffer.size()));

	std::vector<std::byte> result(length);
    uint8_t* const resultData=static_cast<uint8_t*>(static_cast<void*>(result.data()));
	Exception::throwMaybe(EVP_PKEY_encrypt(ctx, resultData, &length, bufferData, buffer.size()));
	assert(length==result.size());

	return result;
}
