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

/************************************************************************/

typedef SteamBot::Modules::Login::Whiteboard::SessionInfo SessionInfo;

/************************************************************************/
/*
 * Get a steam-community URL with the SteamID.
 *
 * For now, I'm assuming they all start like that...
 */

boost::urls::url SteamBot::URLs::getClientCommunityURL()
{
    auto sessionInfo=SteamBot::Client::getClient().whiteboard.has<SessionInfo>();
    if (sessionInfo && sessionInfo->steamId)
    {
        boost::urls::url url("https://steamcommunity.com/profiles");

        url.segments().push_back(std::to_string(sessionInfo->steamId->getValue()));

        return url;
    }
    throw NoURLException();
}
