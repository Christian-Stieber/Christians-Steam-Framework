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

#include "OpenSSL/Exception.hpp"

#include <openssl/evp.h>
#include <cassert>

/************************************************************************/
/*
 * Encrypt/Decrypt the input buffer into the output buffer. Returns
 * the number of bytes written.
 *
 * This performs a full encryption/decryption run -- create context,
 * init with cypher, encrypt/decrypt data, clean up.
 *
 * Note: output buffer must be large enough!
 * Returns the number of bytes actually written (this will be slightly
 * larger than the input size if padding is enabled).
 */

namespace SteamBot
{
    namespace OpenSSL
    {
        class Xcrypt
        {
        public:
            virtual int EVP_XcryptInit_ex(EVP_CIPHER_CTX*, const EVP_CIPHER*, ENGINE*, const unsigned char*, const unsigned char*) const =0;
            virtual int EVP_XcryptUpdate(EVP_CIPHER_CTX*, unsigned char *, int*, const unsigned char*, int) const =0;
            virtual int EVP_XcryptFinal_ex(EVP_CIPHER_CTX*, unsigned char*, int*) const =0;

        public:
            size_t run(const EVP_CIPHER* cipher, bool padding, const std::byte* key, const std::byte* iv,
                       const boost::asio::const_buffer& input, const boost::asio::mutable_buffer& output) const
            {
                auto outputData=static_cast<uint8_t*>(output.data());

                class EVPCipherContext
                {
                private:
                    EVP_CIPHER_CTX* context;

                public:
                    EVPCipherContext()
                    {
                        Exception::throwMaybe(context=EVP_CIPHER_CTX_new());
                    }

                    ~EVPCipherContext()
                    {
                        if (context!=nullptr)
                        {
                            EVP_CIPHER_CTX_free(context);
                        }
                    }

                public:
                    operator decltype(context)() const
                    {
                        return context;
                    }
                };

                EVPCipherContext context;
                {
                    const uint8_t* const keyData=static_cast<const uint8_t*>(static_cast<const void*>(key));
                    const uint8_t* const ivData=static_cast<const uint8_t*>(static_cast<const void*>(iv));
                    Exception::throwMaybe(EVP_XcryptInit_ex(context, cipher, nullptr, keyData, ivData));
                }

                EVP_CIPHER_CTX_set_padding(context, padding);

                int bytesWritten=0;
                Exception::throwMaybe(EVP_XcryptUpdate(context, outputData, &bytesWritten, static_cast<const uint8_t*>(input.data()), static_cast<int>(input.size())));
                outputData+=bytesWritten;

                Exception::throwMaybe(EVP_XcryptFinal_ex(context, outputData, &bytesWritten));
                outputData+=bytesWritten;

                bytesWritten=static_cast<int>(outputData-static_cast<uint8_t*>(output.data()));
                assert(bytesWritten>=0 && static_cast<size_t>(bytesWritten)<=output.size());

                return static_cast<size_t>(bytesWritten);
            }

        public:
            virtual ~Xcrypt() =default;
        };
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace OpenSSL
    {
        class Encrypt : public Xcrypt
        {
        public:
            virtual ~Encrypt() =default;

        public:
            virtual int EVP_XcryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type, ENGINE *impl, const unsigned char *key, const unsigned char *iv) const override
            {
                return EVP_EncryptInit_ex(ctx, type, impl, key, iv);
            }

            virtual int EVP_XcryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl, const unsigned char *in, int inl) const override
            {
                return EVP_EncryptUpdate(ctx, out, outl, in, inl);
            }

            virtual int EVP_XcryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl) const override
            {
                return EVP_EncryptFinal_ex(ctx, out, outl);
            }
        };
    }
}

/************************************************************************/

namespace SteamBot
{
    namespace OpenSSL
    {
        class Decrypt : public Xcrypt
        {
        public:
            virtual ~Decrypt() =default;

        public:
            virtual int EVP_XcryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *type, ENGINE *impl, const unsigned char *key, const unsigned char *iv) const override
            {
                return EVP_DecryptInit_ex(ctx, type, impl, key, iv);
            }

            virtual int EVP_XcryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl, const unsigned char *in, int inl) const override
            {
                return EVP_DecryptUpdate(ctx, out, outl, in, inl);
            }

            virtual int EVP_XcryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl) const override
            {
                return EVP_DecryptFinal_ex(ctx, out, outl);
            }
        };
    }
}
