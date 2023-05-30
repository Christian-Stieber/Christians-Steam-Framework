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
