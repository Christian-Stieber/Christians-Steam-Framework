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

#include "Modules/Login.hpp"
#include "Base64.hpp"

#include <cassert>
#include <boost/json.hpp>

/************************************************************************/

typedef SteamBot::Modules::Login::ParsedToken ParsedToken;

/************************************************************************/

ParsedToken::~ParsedToken() =default;

/************************************************************************/
/*
 * SteamKit Samples/1a.Authentication/Program.cs -> ParseJsonWebToken()
 *
 * Throws if the token can't be parsed properly
 */

ParsedToken::ParsedToken(std::string_view token)
{
    std::string data;
    {
        size_t pos=token.find('.');
        assert(pos!=std::string_view::npos);

        while (++pos<token.size())
        {
            char c=token[pos];

            if (c=='.') break;

            if (c=='-') c='+';
            else if (c=='_') c='/';
            data+=c;
        }
        while (data.size()%4!=0) data+='=';
    }

    const auto bytes=SteamBot::Base64::decode(data);
    json=boost::json::parse(std::string_view((const char*)bytes.data(), bytes.size())).as_object();
}
