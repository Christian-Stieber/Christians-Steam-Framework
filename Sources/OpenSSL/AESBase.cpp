#include "OpenSSL/AES.hpp"
#include "./XCrypt.hpp"

/************************************************************************/
/*
 * Provide the AES encryption for Steam.
 *
 * Basically, the SymmetricEncrypt/SymmetricDecrypt of CryptoHelper in
 * SteamKit
 *
 * The encrypted chunk of data starts with a randomly created IV
 * encrypted via AES/ECB without padding, followed by the actual
 * encrypted data using AES/CBC.
 */

/************************************************************************/
/*
 * https://stackoverflow.com/questions/9889492/how-to-do-encryption-using-aes-in-openssl
 * https://github.com/saju/misc/blob/master/misc/openssl_aes.c
 *
 * https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption
 * https://wiki.openssl.org/index.php/Random_Numbers
 */

typedef SteamBot::OpenSSL::AESCryptoBase AESCryptoBase;

/************************************************************************/
/*
 * Encrypt the iv, then encrypt the payload.
 *
 * See SteamKit2, CryptoHelper.cs -> SymmetricEncryptWithIV
 */

std::vector<std::byte> AESCryptoBase::encryptWithIV(const std::span<const std::byte>& bytes, const IvType& iv) const
{
	// plaintext size, iv size, padding
	std::vector<std::byte> output((bytes.size()+iv.size()+16)&~0x0f);
	auto outputBuffer=boost::asio::buffer(output);

	static const Encrypt xcrypt;

	// encrypt the iv and output it
	auto bytesWritten=xcrypt.run(EVP_aes_256_ecb(), false, key.data(), nullptr, boost::asio::buffer(iv), outputBuffer);
	assert(bytesWritten==iv.size());
	outputBuffer+=bytesWritten;

	// encrypt the data and output it
	bytesWritten+=xcrypt.run(EVP_aes_256_cbc(), true, key.data(), iv.data(), makeBuffer(bytes), outputBuffer);
	assert(bytesWritten<=output.size());
	output.resize(bytesWritten);

	return output;
}

/************************************************************************/
/*
 * Decrypt the iv, decrypt the payload. Returns both.
 *
 * See SteamKit2, CryptoHelper.cs -> SymmetricDecrypt (with the iv param)
 */

std::vector<std::byte> AESCryptoBase::decryptWithIV(const std::span<const std::byte>& bytes, IvType& iv) const
{
	std::vector<std::byte> output(bytes.size()-iv.size());

	static const Decrypt xcrypt;

	// get the iv first
	assert(bytes.size()>=iv.size());
	auto bytesWritten=xcrypt.run(EVP_aes_256_ecb(), false, key.data(), nullptr, boost::asio::const_buffer(bytes.data(), iv.size()), boost::asio::buffer(iv));
	assert(bytesWritten==iv.size());

	// followed by the actual payload
	bytesWritten=xcrypt.run(EVP_aes_256_cbc(), true, key.data(), iv.data(), makeBuffer(bytes.subspan(bytesWritten)), boost::asio::buffer(output));
	assert(bytesWritten<=output.size());
	output.resize(bytesWritten);

	return output;
}
