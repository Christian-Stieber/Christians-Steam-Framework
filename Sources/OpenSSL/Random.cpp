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
	Exception::throwMaybe(RAND_bytes(bufferData, buffer.size()));
}
