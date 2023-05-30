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

#include <openssl/core_names.h>

/************************************************************************/

typedef SteamBot::OpenSSL::AESCryptoHMAC AESCryptoHMAC;

/************************************************************************/
/*
 * HMACSHA1 is a helper class to deal with the HMAC stuff in OpenSSL
 *
 * https://www.openssl.org/docs/man3.0/man3/EVP_MAC.html
 * https://www.openssl.org/docs/manmaster/man7/EVP_MAC-HMAC.html
 */

AESCryptoHMAC::HMACSHA1::HMACSHA1(const KeyType& key)
{
	mac=Exception::throwMaybe(EVP_MAC_fetch(nullptr, "HMAC", nullptr));
	ctx=Exception::throwMaybe(EVP_MAC_CTX_new(mac));

	static_assert(std::tuple_size<KeyType>::value==32);		// we can safely use the first 16 bytes

	static const char digest[]="SHA1";
	OSSL_PARAM params[]={
		{ OSSL_MAC_PARAM_DIGEST, OSSL_PARAM_UTF8_STRING, const_cast<void*>(static_cast<const void*>(digest)), strlen(digest), 0 },
		{ OSSL_MAC_PARAM_KEY, OSSL_PARAM_OCTET_STRING, const_cast<void*>(static_cast<const void*>(key.data())), 16, 0 },
		{ nullptr, 0, nullptr, 0, 0 }
	};
	Exception::throwMaybe(EVP_MAC_CTX_set_params(ctx, params));
}

/************************************************************************/

AESCryptoHMAC::HMACSHA1::~HMACSHA1()
{
	EVP_MAC_CTX_free(ctx);
	if (mac!=nullptr) EVP_MAC_free(mac);
}

/************************************************************************/

size_t AESCryptoHMAC::HMACSHA1::digest(const std::span<boost::asio::const_buffer>& bytes, boost::asio::mutable_buffer result) const
{
	Exception::throwMaybe(EVP_MAC_init(ctx, nullptr, 0, nullptr));
	for (const boost::asio::const_buffer& chunk : bytes)
	{
		Exception::throwMaybe(EVP_MAC_update(ctx, static_cast<const uint8_t*>(chunk.data()), chunk.size()));
	}

	size_t resultSize;
	Exception::throwMaybe(EVP_MAC_final(ctx, nullptr, &resultSize, 0));
	assert(resultSize<=result.size());
	Exception::throwMaybe(EVP_MAC_final(ctx, static_cast<uint8_t*>(result.data()), &resultSize, result.size()));

	return resultSize;
}

/************************************************************************/

AESCryptoHMAC::AESCryptoHMAC(const KeyType& key_)
	: AESCryptoBase(key_),
	  hmacSha1(key_)
{
}

/************************************************************************/

AESCryptoHMAC::~AESCryptoHMAC()
{
}

/************************************************************************/
/*
 * Construct the IV for the AES/HMAC-SHA1 encryption.
 */

void AESCryptoHMAC::makeIV(const std::span<const std::byte>& bytes, IvType& iv) const
{
	/* 3 random bytes */
	std::array<std::byte, 3> random;
	makeRandomBytes(random);

	/* create the HMAC-SHA1 digest of the random bytes followed by the payload */
	std::array<std::byte, 20> hash;
	{
		std::array<boost::asio::const_buffer, 2> source{boost::asio::buffer(random), makeBuffer(bytes)};
		const size_t hashSize=hmacSha1.digest(source, boost::asio::buffer(hash));
		assert(hashSize==hash.size());
	}

	/* now, the 16-byte IV is the first 13 bytes of that hash, and the 3 random bytes */
	const size_t hashPrefixSize=iv.size()-random.size();
	
	std::memcpy(iv.data(), hash.data(), hashPrefixSize);
	std::memcpy(static_cast<std::byte*>(iv.data())+hashPrefixSize, random.data(), random.size());
}

/************************************************************************/

std::vector<std::byte> AESCryptoHMAC::encrypt(const std::span<const std::byte>& bytes) const
{
	IvType iv;
	makeIV(bytes, iv);
	return encryptWithIV(bytes, iv);
}

/************************************************************************/

bool AESCryptoHMAC::isValidIV(const std::span<const std::byte>& plaintext, const IvType& iv) const
{
	/* calculate the hash of the 3 random bytes (extracted from the IV) and the payload */
	std::array<std::byte, 20> hash;
	{
		boost::asio::const_buffer random(static_cast<const std::byte*>(iv.data())+iv.size()-3, 3);
		std::array<boost::asio::const_buffer, 2> source{random, makeBuffer(plaintext)};
		const size_t hashSize=hmacSha1.digest(source, boost::asio::buffer(hash));
		assert(hashSize==hash.size());
	}

	/* and validate that the 13 bytes of the IV match the hash */
	return std::memcmp(hash.data(), iv.data(), iv.size()-3)==0;
}

/************************************************************************/

std::vector<std::byte> AESCryptoHMAC::decrypt(const std::span<const std::byte>& bytes) const
{
	IvType iv;
	auto plaintext=decryptWithIV(bytes, iv);
	if (!isValidIV(std::span<const std::byte>(plaintext), iv))
	{
		throw HMACSHA1MismatchException();
	}
	return plaintext;
}
