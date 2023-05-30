#pragma once

#include "Universe.hpp"

#include <openssl/evp.h>
#include <vector>
#include <span>

/************************************************************************/
/*
 * SteamKit2, CryptoHelper.cs, RSACrypto
 */

namespace SteamBot
{
    namespace OpenSSL
    {
        class RSACrypto
        {
        private:
            EVP_PKEY* pkey=nullptr;
            EVP_PKEY_CTX* ctx=nullptr;

        public:
            RSACrypto(const Universe&);
            ~RSACrypto();

        public:
            std::vector<std::byte> encrypt(const std::span<const std::byte>&);
        };
    }
}
