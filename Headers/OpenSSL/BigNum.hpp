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

#include "./Exception.hpp"

#include <openssl/bn.h>

/************************************************************************/
/*
 * Note: this is an extremely lightweight wrapper for BIGNUM, so no
 * operators, assignment etc.
 */

namespace SteamBot
{
    namespace OpenSSL
    {
        class BigNum
        {
        private:
            BIGNUM* number=Exception::throwMaybe(BN_new());

        public:
            BigNum() =default;
            BigNum(const BigNum&) =delete;
            BigNum(BigNum&&) =delete;

            BigNum(const std::string& string)
            {
                const auto length=BN_hex2bn(&number, string.c_str());
                Exception::throwMaybe(static_cast<size_t>(length)==string.size());
            }

            BigNum(std::span<const std::byte> bytes)
            {
                Exception::throwMaybe(BN_bin2bn(static_cast<const unsigned char*>(static_cast<const void*>(bytes.data())), static_cast<int>(bytes.size()), number));
            }

            ~BigNum()
            {
                BN_free(number);
            }

        public:
            BigNum& operator=(const BigNum&) =delete;
            BigNum& operator=(BigNum&&) =delete;

        public:
            operator BIGNUM*() const
            {
                return number;
            }
        };
    }
}
