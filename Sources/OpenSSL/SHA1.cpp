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
