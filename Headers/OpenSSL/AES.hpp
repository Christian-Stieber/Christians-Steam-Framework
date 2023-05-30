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

#pragma once

#include <openssl/evp.h>

#include <boost/asio/buffer.hpp>

#include <vector>
#include <array>
#include <span>

/************************************************************************/
/*
 * SteamKit2, NetFilterEncryption.cs, NetFilterEncryption
 */

namespace SteamBot
{
    namespace OpenSSL
    {
        class AESCryptoBase
        {
        public:
            static boost::asio::const_buffer makeBuffer(const std::span<const std::byte>& bytes)
            {
                return boost::asio::const_buffer(bytes.data(), bytes.size());
            }

        public:
            typedef std::array<std::byte, 32> KeyType;
            typedef std::array<std::byte, 16> IvType;

        private:
            const KeyType key;

        public:
            AESCryptoBase(const KeyType& myKey)
                : key(myKey)
            {
            }

            virtual ~AESCryptoBase() =default;

        protected:
            std::vector<std::byte> encryptWithIV(const std::span<const std::byte>&, const IvType&) const;
            std::vector<std::byte> decryptWithIV(const std::span<const std::byte>&, IvType&) const;

        public:
            virtual std::vector<std::byte> encrypt(const std::span<const std::byte>&) const =0;
            virtual std::vector<std::byte> decrypt(const std::span<const std::byte>&) const =0;
        };
    }
}

/************************************************************************/
/*
 * "Plain" AES, with a random IV
 *
 * SteamKit2, NetFilterEncryption.cs
 */

namespace SteamBot
{
    namespace OpenSSL
    {
        class AESCrypto : public AESCryptoBase
        {
        public:
            AESCrypto(const KeyType& myKey)
                : AESCryptoBase(myKey)
            {
            }

            virtual ~AESCrypto() =default;

        public:
            virtual std::vector<std::byte> encrypt(const std::span<const std::byte>&) const override;
            virtual std::vector<std::byte> decrypt(const std::span<const std::byte>&) const override;
        };
    }
}

/************************************************************************/
/*
 * AES, with HMAC-SHA1
 *
 * SteamKit2, NetFilterEncryptionWithHMAC.cs
 */

namespace SteamBot
{
    namespace OpenSSL
    {
        class AESCryptoHMAC : public AESCryptoBase
        {
        private:
            class HMACSHA1
            {
            private:
                EVP_MAC* mac=nullptr;
                EVP_MAC_CTX* ctx=nullptr;

            public:
                HMACSHA1(const KeyType&);
                size_t digest(const std::span<boost::asio::const_buffer>&, boost::asio::mutable_buffer) const;
                ~HMACSHA1();
            };

            const HMACSHA1 hmacSha1;

        public:
            class HMACSHA1MismatchException : public std::exception
            {
            public:
                virtual const char* what() const noexcept override
                {
                    return "HMAC-SHA1 mismatch on received data";
                }
            };

        public:
            AESCryptoHMAC(const KeyType&);
            virtual ~AESCryptoHMAC();

        private:
            void makeIV(const std::span<const std::byte>&, IvType&) const;
            bool isValidIV(const std::span<const std::byte>&, const IvType&) const;

        public:
            virtual std::vector<std::byte> encrypt(const std::span<const std::byte>&) const override;
            virtual std::vector<std::byte> decrypt(const std::span<const std::byte>&) const override;
        };
    }
}
