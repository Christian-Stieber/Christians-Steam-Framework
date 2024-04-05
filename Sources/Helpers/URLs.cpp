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

#include "Helpers/URLs.hpp"
#include "Modules/Login.hpp"
#include "Client/Client.hpp"

#include <charconv>

/************************************************************************/
/*
 * Get a steam-community URL with the SteamID.
 *
 * For now, I'm assuming they all start like that...
 */

boost::urls::url SteamBot::URLs::getClientCommunityURL()
{
    if (auto steamId=SteamBot::Client::getClient().whiteboard.has<SteamBot::Modules::Login::Whiteboard::SteamID>())
    {
        boost::urls::url url("https://steamcommunity.com/profiles");
        url.segments().push_back(std::to_string(steamId->getValue()));
        url.params().set("l", "english");
        return url;
    }
    throw NoURLException();
}

/************************************************************************/

template <std::integral T> void SteamBot::URLs::setParam_(boost::urls::url& url, const char* name, T value)
{
    char string[32];
    auto result=std::to_chars(string, string+sizeof(string), value);
    assert(result.ec==std::errc());
    url.params().set(name, std::string_view(string, result.ptr));
}

template void SteamBot::URLs::setParam_(boost::urls::url&, const char*, long long);
template void SteamBot::URLs::setParam_(boost::urls::url&, const char*, unsigned long long);
