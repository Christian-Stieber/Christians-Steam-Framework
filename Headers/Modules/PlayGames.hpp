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

/************************************************************************/
/*
 * The PlayGames module lets you start and stop "playing" games.
 *
 * This module will also attempt to keep OwnedGames playtime data
 * updated. However, see OwnedGames -> GameChanged for playtime
 * accuracy concerns; if you want more accurate data, you'll need to
 * track it yourself and just sync up with OwnedGames updates if and
 * when they come in.
 *
 * Also note that we cannot currently(?) track what Steam ACTUALLY
 * considers being played: I'm merely assuming it does what I tell it.
 * You can make this more reliable by limiting the game count to a
 * reasonably low-ish number.
 */

/************************************************************************/

#pragma once

#include "MiscIDs.hpp"

#include <vector>

#include <boost/json/value.hpp>

/************************************************************************/
/*
 * Send this message to start/stop "playing" a game
 */

namespace SteamBot
{
    namespace Modules
    {
        namespace PlayGames
        {
            namespace Messageboard
            {
                class PlayGame
                {
                public:
                    AppID appId=AppID::None;
                    bool start=true;

                public:
                    PlayGame();
                    ~PlayGame();
                    boost::json::value toJson() const;

                public:
                    static void play(AppID, bool);
                };
            }
        }
    }
}

/************************************************************************/
/*
 * This provides the list of games that are currently "played" by
 * our module.
 */

namespace SteamBot
{
    namespace Modules
    {
        namespace PlayGames
        {
            namespace Whiteboard
            {
                class PlayingGames : public std::vector<SteamBot::AppID>
                {
                public:
                    PlayingGames(const std::vector<SteamBot::AppID>&);
                    ~PlayingGames();
                };
            }
        }
    }
}
