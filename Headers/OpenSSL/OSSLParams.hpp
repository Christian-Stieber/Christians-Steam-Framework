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

#include <openssl/param_build.h>

/************************************************************************/

namespace SteamBot
{
    namespace OpenSSL
    {
        class OSSLParams
        {
        private:
            OSSL_PARAM_BLD* build=Exception::throwMaybe(OSSL_PARAM_BLD_new());
            OSSL_PARAM* params=nullptr;

        public:
            template <typename FUNC> OSSLParams(FUNC populate)
            {
                populate(build);
                params=OSSL_PARAM_BLD_to_param(build);
            }

            ~OSSLParams()
            {
                OSSL_PARAM_free(params);
                OSSL_PARAM_BLD_free(build);
            }

        public:
            operator OSSL_PARAM*() const
            {
                return params;
            }
        };
    }
}
