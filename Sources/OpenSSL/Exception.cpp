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

#include "OpenSSL/Exception.hpp"

#include <openssl/err.h>
#include <openssl/bio.h>

#include <boost/log/trivial.hpp>

/************************************************************************/

typedef SteamBot::OpenSSL::Exception Exception;

/************************************************************************/

static void logErrors()
{
	static const bool initialized=[](){
        ERR_load_crypto_strings();
        return true;
    }();

    assert(ERR_peek_error()!=0);

	BIO* bio=BIO_new(BIO_s_mem());
	ERR_print_errors(bio);
    {
        const auto written=BIO_write(bio, "", 1);
        assert(written==1);
    }
    char* data;
    const auto size=BIO_get_mem_data(bio, &data);

    BOOST_LOG_TRIVIAL(error) << "OpenSSL error: " << std::string_view(data, size);

    BIO_free(bio);
}

/************************************************************************/

Exception::Exception()
{
    logErrors();
}

/************************************************************************/

Exception::~Exception() =default;

/************************************************************************/

void Exception::throwMaybe()
{
	if (ERR_peek_error()!=0)
	{
		throw Exception();
	}
}

/************************************************************************/

void Exception::throwMaybe(int result)
{
	if (result<=0)
	{
		throw Exception();
	}
}
